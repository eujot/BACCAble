"""Audit lighting chronology without changing original captures or assigning ECU identities."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sqlite3
import statistics

FIELDS = {
 ('BH',0x738,8): [('channels',lambda d:list(d[1:3]))],
 ('C1',0x738,5): [('payload',lambda d:d.hex())],
 ('BH',0x354,4): [('dipped',lambda d:int(bool(d[1]&64))),('auto',lambda d:int(bool(d[3]&64)))],
 ('C2',0x73e,4): [('dipped',lambda d:int(bool(d[1]&64))),('auto',lambda d:int(bool(d[3]&64)))],
 ('C1',0x73e,4): [('position',lambda d:int(bool(d[0]&2)))],
 ('C1',0x384,8): [('dipped',lambda d:int(bool(d[3]&128)))],
 ('C2',0x384,8): [('dipped',lambda d:int(bool(d[3]&128)))],
 ('C2',0x42d,5): [('lamp_code',lambda d:d[0])],
 ('C2',0x42e,5): [('lamp_code',lambda d:d[0])],
}
DETAIL = {'20260928-204607Z','20261010-104325Z','20261010-110821Z',
          '20261024-105643Z','20261024-111510Z','20261024-113104Z','20261024-113644Z'}

def stats(values):
    values=sorted(values)
    if not values:return None
    return {'n':len(values),'min':values[0],'p05':values[int((len(values)-1)*.05)],
            'median':statistics.median(values),'p95':values[int((len(values)-1)*.95)],'max':values[-1]}

def fingerprints(directory):
    result={}
    for name in ['manifest.json','summary.json','session.sqlite3','session.sqlite3-wal','C1.bin','C2.bin','BH.bin']:
        p=directory/name
        if p.exists():
            with p.open('rb') as source:
                result[name]={'size':p.stat().st_size,'sha256':hashlib.file_digest(source,'sha256').hexdigest()}
    return result

def inventory_sessions(root):
    """Retain capture metadata, original markers and hashes before analysis."""
    result = []
    for database in sorted(root.glob('*/session.sqlite3')):
        directory = database.parent
        initial = fingerprints(directory)
        connection = sqlite3.connect(database.as_uri() + '?mode=ro', uri=True)
        shapes = connection.execute('SELECT role,dlc,count(*) FROM can_frames WHERE arbitration_id=1848 GROUP BY role,dlc').fetchall()
        events = [dict(zip(['id','host_ns','label','source','note'], row)) for row in
                  connection.execute('SELECT id,host_ns,label,source,note FROM events ORDER BY host_ns,id')]
        connection.close()
        result.append({'session': directory.name,
                       'summary': json.loads((directory/'summary.json').read_text()),
                       'manifest': json.loads((directory/'manifest.json').read_text()),
                       'frame_738_shapes': shapes, 'events': events,
                       'input_fingerprints': initial})
    return result


def analyze(directory, initial):
    c=sqlite3.connect((directory/'session.sqlite3').as_uri()+'?mode=ro',uri=True)
    c.execute('BEGIN')
    result=dict(initial)
    groups=defaultdict(list)
    ids=sorted({key[1] for key in FIELDS})
    sql=('SELECT id,sequence,host_ns,device_timestamp_ms,role,arbitration_id,dlc,data '
         'FROM can_frames WHERE arbitration_id IN ('+','.join('?' for _ in ids)+') ORDER BY role,sequence')
    for row in c.execute(sql,ids):
        if (row[4],row[5],row[6]) in FIELDS:groups[row[4],row[5],row[6]].append(row)
    batches={}
    if directory.name in DETAIL:
        for role,host,first,last,count in c.execute('SELECT role,host_ns,min(device_timestamp_ms),max(device_timestamp_ms),count(*) FROM can_frames WHERE host_ns>=2000000000 GROUP BY role,host_ns'):
            if 0<=last-first<=1000:batches[role,host]=(first,last,count)
        result['usb_batches']={}
        for role in ['C1','C2','BH']:
            entries=[(h,*v) for (r,h),v in batches.items() if r==role]
            result['usb_batches'][role]={'span_ms':stats([v[2]-v[1] for v in entries]),
                                        'frames_per_read':stats([v[3] for v in entries]),
                                        'endpoint_host_minus_device_ms':stats([round(v[0]/1e6-v[2],3) for v in entries])}
    # Fit each clock to USB read endpoints; this estimates drift, not wire latency.
    clock_fits = {}
    if batches:
        result['endpoint_clock_fit'] = {}
        for role in ['C1','C2','BH']:
            points = [(v[1], h/1e6) for (r,h),v in batches.items() if r == role]
            if len(points) < 2:
                continue
            mean_device = statistics.mean(p[0] for p in points)
            mean_host = statistics.mean(p[1] for p in points)
            variance = sum((d-mean_device)**2 for d,h in points)
            if not variance:
                continue
            slope = sum((d-mean_device)*(h-mean_host) for d,h in points)/variance
            intercept = mean_host-slope*mean_device
            residuals = [h-(slope*d+intercept) for d,h in points]
            clock_fits[role] = (slope, intercept)
            result['endpoint_clock_fit'][role] = {
                'host_per_device': slope, 'intercept_ms': intercept,
                'apparent_drift_ppm': (slope-1)*1e6,
                'residual_ms': stats(residuals),
                'limit': 'Endpoint least-squares fit removes average drift; constant role-specific transport delay is inseparable from clock offset. Residuals are not a bound on wire-time error.'}
    def sample(row):
        b=batches.get((row[4],row[2]))
        return {'frame_id':row[0],'sequence':row[1],'host_ms':round(row[2]/1e6,3),
                'device_ms':row[3],'data':row[7].hex(),
                'batch_adjusted_host_ms':round(row[2]/1e6-(b[1]-row[3]),3) if b else None,
                'endpoint_fit_host_ms':round(clock_fits[row[4]][0]*row[3]+clock_fits[row[4]][1],3) if row[4] in clock_fits else None}
    profiles=[];traces={}
    for key,rows in sorted(groups.items()):
        valid=[r for r in rows if r[2]>=2e9]
        periods=[b[3]-a[3] for a,b in zip(valid,valid[1:])]
        p={'role':key[0],'can_id':f'0x{key[1]:03X}','dlc':key[2],'frames':len(rows),
           'period_ms_after_startup':stats(periods),'fields':{}}
        for name,decode in FIELDS[key]:
            changes=[];prev=None
            for r in valid:
                value=decode(r[7])
                if value!=prev:
                    changes.append(dict(sample(r),value=value));prev=value
            p['fields'][name]={'values':dict(Counter(str(decode(r[7])) for r in valid)), 'changes':changes}
            traces[key+(name,)]=changes
        if key==('BH',0x738,8):
            p['byte_values']=[dict(Counter(str(r[7][b]) for r in valid)) for b in range(8)]
            p['beta34_guard_rejected']=sum(r[7][1]>7 or r[7][2]>7 or r[7][6]!=0 or r[7][7]!=0 for r in valid)
            runs=[];last=None;start=None;n=0
            for r in valid:
                value=r[7][4]>>5
                if value!=last:
                    if n:runs.append({'value':last,'frames':n,'duration_ms':previous[3]-start[3],'start':sample(start)})
                    last=value;start=r;n=0
                previous=r;n+=1
            if n:runs.append({'value':last,'frames':n,'duration_ms':previous[3]-start[3],'start':sample(start)})
            p['byte4_high3_longest_plateaus']=sorted(runs,key=lambda x:x['duration_ms'],reverse=True)[:3]
            p['byte4_high3_step_histogram']=dict(Counter(str(((b[7][4]>>5)-(a[7][4]>>5))%8) for a,b in zip(valid,valid[1:])))
        profiles.append(p)
    result['profiles']=profiles
    if directory.name in DETAIL:
        base=traces.get(('BH',0x354,4,'dipped'),[])
        result['lamp_edges']=[]
        for edge in base[1:]:
            peers={}
            for key in [('C1',0x73e,4,'position'),('C2',0x73e,4,'dipped'),('C1',0x384,8,'dipped'),('C2',0x384,8,'dipped'),('C2',0x42d,5,'lamp_code'),('C2',0x42e,5,'lamp_code')]:
                want=(0x25 if edge['value'] else 0x44) if key[3]=='lamp_code' else edge['value']
                matches=[e for e in traces.get(key,[])[1:] if e['value']==want and abs(e['host_ms']-edge['host_ms'])<1000]
                if matches:
                    e=min(matches,key=lambda x:abs(x['host_ms']-edge['host_ms']))
                    peers[f'{key[0]}:0x{key[1]:03X}']=dict(e,host_delta_from_bh_ms=round(e['host_ms']-edge['host_ms'],3),
                        adjusted_delta_from_bh_ms=round(e['batch_adjusted_host_ms']-edge['batch_adjusted_host_ms'],3) if e['batch_adjusted_host_ms'] is not None and edge['batch_adjusted_host_ms'] is not None else None)
            sensors=[e for e in traces.get(('BH',0x738,8,'channels'),[]) if e['device_ms']<=edge['device_ms']]
            autos=[e for e in traces.get(('BH',0x354,4,'auto'),[]) if e['device_ms']<=edge['device_ms']]
            result['lamp_edges'].append({'bh':edge,'peers':peers,'sensor_changes_before':sensors[-4:],
                'auto':autos[-1] if autos else None})
        c2=traces.get(('C2',0x73e,4,'dipped'),[])
        result['c2_lamp_order']=[]
        for edge in c2[1:]:
            peers={}
            for key in [('C2',0x384,8,'dipped'),('C2',0x42d,5,'lamp_code'),('C2',0x42e,5,'lamp_code')]:
                want=(0x25 if edge['value'] else 0x44) if key[3]=='lamp_code' else edge['value']
                matches=[e for e in traces.get(key,[])[1:] if e['value']==want and abs(e['device_ms']-edge['device_ms'])<1000]
                if matches:
                    e=min(matches,key=lambda x:abs(x['device_ms']-edge['device_ms']))
                    peers[f'0x{key[1]:03X}']=dict(e,device_delta_ms=e['device_ms']-edge['device_ms'])
            result['c2_lamp_order'].append({'reference':edge,'peers':peers})
    # Verify each retained change directly against its original 16-byte record.
    checked = 0
    handles = {}
    try:
        frame_ids = {entry['frame_id'] for changes in traces.values() for entry in changes}
        for frame_id in sorted(frame_ids):
            role, offset, raw_time, can_id, dlc, data = c.execute(
                'SELECT role,raw_offset,device_timestamp_raw,arbitration_id,dlc,data FROM can_frames WHERE id=?',
                (frame_id,)).fetchone()
            if role not in handles:
                handles[role] = (directory/(role+'.bin')).open('rb')
            # raw_offset identifies the USB read, not the exact record start;
            # a record may begin in the previous read's final 15 bytes.
            handles[role].seek(max(0, offset-15))
            raw = handles[role].read(4096+30)
            expected = bytes([0xA0+dlc]) + raw_time.to_bytes(3, 'little') + can_id.to_bytes(4, 'little') + data
            if expected not in raw:
                raise ValueError(f'Raw mismatch: {directory.name} frame {frame_id}')
            checked += 1
    finally:
        for handle in handles.values():
            handle.close()
    result['raw_change_records_verified'] = checked
    c.close()
    if fingerprints(directory)!=initial['input_fingerprints']:raise ValueError('Original inputs changed: '+directory.name)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions',type=Path,required=True)
    parser.add_argument('--inventory',type=Path,help='Optional previously fingerprinted inventory')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); root=args.sessions.resolve(); output=args.output.resolve()
    if output.exists() or output.is_relative_to(root):parser.error('Use a new output outside the captures')
    inventory=json.loads(args.inventory.read_text()) if args.inventory else inventory_sessions(root);results=[]
    for item in inventory:
        print(item['session'],flush=True)
        results.append(analyze(root/item['session'],item))
    output.write_text(json.dumps({'method':'Exact bus/ID/DLC; same-board device clock; host USB batching audited; no ECU source attribution from timestamps alone.',
      'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'sessions':results},indent=2)+'\n')

if __name__=='__main__':main()
