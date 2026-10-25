"""Find repeated bit steps and short excursions near lamp transitions on all recorded buses."""
import argparse,hashlib,json,sqlite3
from analyze_light_origin import fingerprints
from collections import Counter,defaultdict
from pathlib import Path


def scan(root,evidence):
    results=[]
    for session in evidence['sessions']:
        sid=session['session']
        if sid not in ['20261010-104325Z','20261024-105643Z','20261024-111510Z']:continue
        c=sqlite3.connect((root/sid/'session.sqlite3').as_uri()+'?mode=ro',uri=True)
        edges=session['lamp_edges']
        if sid=='20261010-104325Z':edges=[e for e in edges if 840000<e['bh']['host_ms']<862000]
        supported=defaultdict(list);pulses=defaultdict(list);rare=[]
        for ix,edge in enumerate(edges):
            refs={'BH':edge['bh'],'C1':edge['peers'].get('C1:0x73E'),'C2':edge['peers'].get('C2:0x73E')}
            groups=defaultdict(list);host=edge['bh']['host_ms']
            for row in c.execute('SELECT role,arbitration_id,dlc,id,sequence,host_ns,device_timestamp_ms,data FROM can_frames WHERE host_ns BETWEEN ? AND ? ORDER BY role,sequence',(int((host-3500)*1e6),int((host+2000)*1e6))):
                role,canid,dlc,fid,seq,h,d,data=row
                if not refs[role] or not (0<dlc<=8):continue
                rel=d-refs[role]['device_ms']
                if -3200<=rel<=1700:groups[role,canid,dlc].append((rel,fid,seq,h,d,data))
            for key,rows in groups.items():
                pre=[r for r in rows if -2000<=r[0]<=-1000]
                post=[r for r in rows if 400<=r[0]<=1400]
                if not pre or not post:
                    if any(-1500<=r[0]<=200 for r in rows):rare.append({'edge':ix,'role':key[0],'can_id':hex(key[1]),'dlc':key[2],'pre':len(pre),'post':len(post),'near':[(r[0],r[1],r[5].hex()) for r in rows if -1500<=r[0]<=200][:8]})
                    continue
                for byte in range(key[2]):
                    before=Counter(r[5][byte] for r in pre);after=Counter(r[5][byte] for r in post)
                    for bit in range(8):
                        mask=1<<bit
                        a=sum(n for v,n in before.items() if v&mask)/len(pre)
                        b=sum(n for v,n in after.items() if v&mask)/len(post)
                        field=key+(byte,bit)
                        if (a<=.05 and b>=.95) or (a>=.95 and b<=.05):
                            value=int(b>=.95)
                            changes=[]
                            for previous,current in zip(rows,rows[1:]):
                                if -1500<=current[0]<=1400 and bool(previous[5][byte]&mask)!=bool(current[5][byte]&mask) and bool(current[5][byte]&mask)==bool(value):changes.append(current)
                            if changes:
                                transition=min(changes,key=lambda r:abs(r[0]))
                                supported[field+(int(value==edge['bh']['value']),)].append({'edge':ix,'lamp_value':edge['bh']['value'],'relative_device_ms':transition[0],'frame_id':transition[1],'before_fraction':a,'after_fraction':b,'pre_samples':len(pre),'post_samples':len(post)})
                        elif (a<=.05 and b<=.05) or (a>=.95 and b>=.95):
                            baseline=int(a>=.95);excursions=[]
                            for previous,current in zip(rows,rows[1:]):
                                if -1500<=current[0]<=200 and bool(previous[5][byte]&mask)==bool(baseline) and bool(current[5][byte]&mask)!=bool(baseline):excursions.append(current)
                            if excursions:
                                pulses[field].append({'edge':ix,'lamp_value':edge['bh']['value'],'excursions':len(excursions),'first_ms':excursions[0][0],'first_frame_id':excursions[0][1]})
        def summarise(items):
            return [{'role':k[0],'can_id':f'0x{k[1]:03X}','dlc':k[2],'byte':k[3],'bit':k[4],'same_polarity_as_lamp':bool(k[5]) if len(k)>5 else None,'hits':len(v),'of_edges':len(edges),'matches':v} for k,v in sorted(items.items(),key=lambda x:(-len(x[1]),x[0])) if len(v)>=3]
        results.append({'session':sid,'edge_count':len(edges),'level_steps':summarise(supported),'short_excursions':summarise(pulses),'rare_shapes':rare})
        c.close()
        if fingerprints(root/sid) != session['input_fingerprints']:
            raise ValueError('Original inputs changed: ' + sid)
        print(sid,len(edges),'edges',len(results[-1]['level_steps']),'level candidates',len(results[-1]['short_excursions']),'pulse candidates',flush=True)
    return results

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions',type=Path,required=True);parser.add_argument('--evidence',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    a=parser.parse_args()
    if a.output.exists() or a.output.resolve().is_relative_to(a.sessions.resolve()):parser.error('Use new output outside original captures')
    out=scan(a.sessions.resolve(),json.loads(a.evidence.read_text()))
    a.output.write_text(json.dumps({'method':'Per-board lamp reference; pre [-2000,-1000] and post [400,1400] ms; bit occupancy <=5% / >=95%; excursions [-1500,200] ms; no semantic promotion from score alone.','script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'evidence_sha256':hashlib.sha256(a.evidence.read_bytes()).hexdigest(),'sessions':out},indent=2)+'\n')
