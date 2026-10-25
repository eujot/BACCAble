"""Inspect manual lamp trials, including brief precursors and stable bit controls."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sqlite3
from analyze_light_origin import fingerprints


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sessions', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = args.sessions.resolve()
    if args.output.exists() or args.output.resolve().is_relative_to(root):
        parser.error('Use a fresh output outside original captures')
    evidence = json.loads(args.evidence.read_text())
    session = next(s for s in evidence['sessions'] if s['session'] == '20261010-104325Z')
    directory = root/session['session']
    before = fingerprints(directory)
    if before != session['input_fingerprints']:
        raise ValueError('Original capture differs from prior evidence')
    db = sqlite3.connect((directory/'session.sqlite3').as_uri()+'?mode=ro', uri=True)
    groups = defaultdict(list)
    for row in db.execute('SELECT role,arbitration_id,dlc,id,sequence,host_ns,device_timestamp_ms,data,raw_offset,device_timestamp_raw FROM can_frames WHERE host_ns BETWEEN 780000000000 AND 865000000000 ORDER BY role,sequence'):
        groups[row[0],row[1],row[2]].append(row)
    edges = [e for e in session['lamp_edges'] if 840000 < e['bh']['host_ms'] < 862000]
    retained = set()
    def sample(row, reference=None):
        retained.add(row[3])
        return {'frame_id':row[3], 'sequence':row[4], 'host_ms':row[5]/1e6,
                'device_ms':row[6], 'data':row[7].hex(),
                'relative_device_ms':row[6]-reference if reference is not None else None}
    trials = []
    candidates = defaultdict(list)
    for index, edge in enumerate(edges):
        refs = {'BH':edge['bh'], 'C1':edge['peers']['C1:0x73E'], 'C2':edge['peers']['C2:0x73E']}
        trial = {'index':index, 'lamp_on':bool(edge['bh']['value']), 'references':refs, 'reports':{}, 'sparse_shapes':[]}
        for key, rows in groups.items():
            ref = refs[key[0]]['device_ms']
            nearby = [r for r in rows if -2000 <= r[6]-ref <= 1500]
            pre = [r for r in nearby if -1800 <= r[6]-ref <= -800]
            near = [r for r in nearby if -750 <= r[6]-ref <= 100]
            if key in [('BH',0x354,4),('C2',0x73e,4),('C1',0x73e,4),('BH',0x738,8)]:
                changes=[];previous=None
                for r in nearby:
                    if r[7]!=previous: changes.append(sample(r,ref));previous=r[7]
                trial['reports'][f'{key[0]}:0x{key[1]:03X}/{key[2]}'] = changes
            if not pre:
                if near: trial['sparse_shapes'].append({'role':key[0],'can_id':hex(key[1]),'dlc':key[2], 'pre_samples':len(pre),'near':[sample(r,ref) for r in near]})
                continue
            # Require an exactly stable one-second baseline, retaining every bit.
            for byte in range(key[2]):
                for bit in range(8):
                    mask=1<<bit
                    baseline={bool(r[7][byte]&mask) for r in pre}
                    if len(baseline)!=1: continue
                    value=baseline.pop()
                    changed=[r for r in near if bool(r[7][byte]&mask)!=value]
                    if not changed:continue
                    first=changed[0]
                    # Confirm a transition, rather than treating a window boundary as onset.
                    previous=[r for r in nearby if r[6]<first[6]]
                    if not previous or bool(previous[-1][7][byte]&mask)!=value:continue
                    candidates[key+(byte,bit,edge['bh']['value'])].append({'trial':index,'baseline':int(value),'pre_samples':len(pre),'onset':sample(first,ref)})
        trials.append(trial)
    ranked=[]
    for key,hits in candidates.items():
        if len(hits)<2:continue
        rows=groups[key[:3]];mask=1<<key[4]
        controls=[r for r in rows if 815e9 <= r[5] <= 840e9]
        control_changes=sum(bool(a[7][key[3]]&mask)!=bool(b[7][key[3]]&mask) for a,b in zip(controls,controls[1:]))
        ranked.append({'role':key[0],'can_id':f'0x{key[1]:03X}','dlc':key[2],'byte':key[3],'bit':key[4],
                       'lamp_on':bool(key[5]),'hits':len(hits),'of_trials':3,'control_changes_815_840s':control_changes,'matches':hits})
    ranked.sort(key=lambda c:(not c['lamp_on'],-c['hits'],c['control_changes_815_840s'],c['role'],c['can_id'],c['byte'],c['bit']))
    checked=0
    for rows in groups.values():
        for r in rows:
            if r[3] not in retained:continue
            with (directory/(r[0]+'.bin')).open('rb') as stream:
                stream.seek(max(0,r[8]-15));raw=stream.read(4096+30)
            expected=bytes([0xa0+r[2]])+r[9].to_bytes(3,'little')+r[1].to_bytes(4,'little')+r[7]
            if expected not in raw:raise ValueError('Binary mismatch '+str(r[3]))
            checked+=1
    db.close()
    if fingerprints(directory)!=before:raise ValueError('Capture changed')
    result={'session':session['session'],'input_fingerprints':before,'summary':session['summary'],
            'events':[e for e in session['events'] if 61<=e['id']<=78],
            'method':'All bus/ID/DLC shapes; exact stable bit baseline [-1800,-800] ms; excursions [-750,+100] ms relative to same-board lamp report. Single-sample baselines allowed to retain slow signals; support counts retained. Negative control 815..840 host seconds includes brake trials. Sparse shapes retained. Scores do not establish causation.',
            'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'trials':trials,'candidates':ranked,'raw_records_verified':checked}
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Retained raw records verified:',checked)
    for item in ranked:
        if item['lamp_on'] and item['hits']==3:
            print(item['role'],item['can_id'],item['byte'],item['bit'],'control',item['control_changes_815_840s'], 'onsets',[h['onset']['relative_device_ms'] for h in item['matches']])

if __name__=='__main__': main()
