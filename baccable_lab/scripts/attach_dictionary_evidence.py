"""Attach reproducible aggregate evidence to the curated dictionary (no session edits)."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'baccable_lab'))
from baccable_lab.events.catalog import EVENT_GROUPS


def attach(dictionary: dict, report: dict, report_sha256: str,
           report_path: str = 'docs/research/20260928-session-review.json') -> dict:
    if (dictionary['dictionary_version'],dictionary['firmware_source_commit']) != (
            report['dictionary_version'],report['firmware_source_commit']):
        raise ValueError('report and dictionary source versions differ')
    prior_sessions = {s['session'] for s in dictionary.get('session_evidence',{}).get('sessions',[])}
    new_sessions = {s['session'] for s in report['sessions']}
    if prior_sessions - new_sessions:
        raise ValueError('report would discard previous session evidence; use a cumulative report')
    dictionary['session_evidence'] = {
        'report': report_path, 'report_sha256': report_sha256,
        'sessions': [{k: s[k] for k in ('session','status','duration_seconds','frames','input_sha256')}
                     for s in report['sessions']],
        'total_frames': report['total_frames'], 'manual_events': report['total_manual_events'],
        'derived_events': report['total_derived_events'],
        'limits': 'Independent marker agreement supports a candidate; missing transitions, conflicting labels and RX-only capture prevent universal validation.'}
    for message in dictionary['messages']:
        observations = []
        for session in report['sessions']:
            frames = sum(i['frames'] for i in session['inventory']
                         if (i['role'],i['can_id']) == (message['role'],message['can_id']))
            if frames:
                observations.append({'session':session['session'],'frames':frames})
        message['observations'] = observations
        for signal in message['signals']:
            correlations = []
            for session in report['sessions']:
                matches = [c for c in session['marker_correlations']
                           if (c['role'],c['can_id'],c['signal']) == (message['role'],message['can_id'],signal['name'])]
                if matches:
                    correlations.append({'session':session['session'],
                                         'results':dict(Counter(c['result'] for c in matches)),
                                         'event_ids':[c['event_id'] for c in matches]})
            signal.pop('manual_marker_evidence', None)
            if correlations:
                signal['manual_marker_evidence'] = correlations
    links = []
    for _, (group, markers) in EVENT_GROUPS.items():
        for label,title in markers:
            linked = [{'role':m['role'],'can_id':m['can_id'],'signal':s['name'],'origin':s['origin']}
                      for m in dictionary['messages'] for s in m['signals'] if label in s.get('marker_rules',{})]
            observed = [{'session':session['session'],'event_id':event['id'],'source':event['source']}
                        for session in report['sessions'] for event in session['events'] if event['label']==label]
            links.append({'label':label,'title':title,'group':group,'candidate_signals':linked,
                          'observed_events':observed,'status':'candidate_rules' if linked else 'no_event_decoder'})
    existing = {x['label'] for x in links}
    for label in sorted({e['label'] for s in report['sessions'] for e in s['events']} - existing):
        links.append({'label':label,'title':label,'group':'Recorded non-catalog events','candidate_signals':[],
                      'observed_events':[{'session':s['session'],'event_id':e['id'],'source':e['source']}
                                         for s in report['sessions'] for e in s['events'] if e['label']==label],
                      'status':'derived_not_independent' if all(e['source'].startswith('analysis:') for s in report['sessions'] for e in s['events'] if e['label']==label) else 'unresolved'})
    dictionary['marker_index'] = links
    dictionary['frame_templates'] = report.get('frame_templates', [])
    inventory = {}
    for session in report['sessions']:
        for item in session['inventory']:
            key = (item['role'],item['can_id'],item['dlc'])
            if key not in inventory:
                inventory[key] = {k:item[k] for k in ('role','can_id','dlc','title','coverage')}
                inventory[key].update(frames=0,sessions=[])
            inventory[key]['frames'] += item['frames']
            inventory[key]['sessions'].append(session['session'])
    dictionary['observed_inventory'] = [inventory[k] for k in sorted(inventory)]
    return dictionary


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    path = ROOT / 'baccable_lab/baccable_lab/knowledge/giulia.json'
    data = args.report.read_bytes()
    report_path = args.report.resolve().relative_to(ROOT / 'baccable_lab').as_posix()
    dictionary = attach(json.loads(path.read_text()),json.loads(data),hashlib.sha256(data).hexdigest(), report_path)
    path.write_text(json.dumps(dictionary,indent=2)+'\n')
