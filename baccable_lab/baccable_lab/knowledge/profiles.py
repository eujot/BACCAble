"""Empirical frame templates and exploratory bit steps, without semantic inference."""
from bisect import bisect_left, bisect_right
from collections import Counter, defaultdict


def profile_frames(database, catalog, events):
    """One bounded-memory pass. Device intervals are local to a board, never cross-board."""
    profiles = {}
    markers = sorted((e for e in events if e['source'] in ('keyboard', 'cli', 'scenario')), key=lambda e:e['host_ns'])
    times = [e['host_ns'] for e in markers]
    windows = defaultdict(lambda: [Counter(), Counter()])
    for role, can_id, dlc, host, device, payload in database.execute(
            'SELECT role,arbitration_id,dlc,host_ns,device_timestamp_ms,data FROM can_frames ORDER BY role,sequence'):
        data = bytes(payload)[:dlc]
        if len(data) != dlc or not 0 <= dlc <= 8:
            continue
        key = role, can_id, dlc
        value = int.from_bytes(data, 'big')
        if key not in profiles:
            profiles[key] = {'first':value, 'volatile':0, 'frames':0, 'last_device':None,
                             'periods':Counter(), 'payloads':set(), 'capped':False}
        p = profiles[key]
        p['frames'] += 1
        p['volatile'] |= p['first'] ^ value
        if p['last_device'] is not None and device >= p['last_device']:
            p['periods'][device-p['last_device']] += 1
        p['last_device'] = device
        if len(p['payloads']) < 64:
            p['payloads'].add(value)
        elif value not in p['payloads']:
            p['capped'] = True
        # Compare stable baseline -5..-3 s against -0.5..+1.5 s around a manual marker.
        # Human reaction delay is unknown; these are exploratory windows, not event times.
        if can_id > 0x7FF:
            continue
        for index in range(bisect_left(times,host-1_500_000_000), bisect_right(times,host+5_000_000_000)):
            delta = host-times[index]
            phase = 0 if -5_000_000_000 <= delta <= -3_000_000_000 else 1 if -500_000_000 <= delta <= 1_500_000_000 else None
            if phase is not None:
                for byte, raw in enumerate(data):
                    windows[(index,role,can_id,dlc,byte)][phase][raw] += 1
    observations = []
    for (role,can_id,dlc), p in sorted(profiles.items()):
        mask = ((1 << (8*dlc))-1) ^ p['volatile']
        observations.append({'role':role,'can_id':f'0x{can_id:03X}','dlc':dlc,'frames':p['frames'],
                             'payload_constant':p['volatile']==0,
                             'distinct_payloads':len(p['payloads']), 'distinct_count_is_lower_bound':p['capped'],
                             'volatile_bit_masks':p['volatile'].to_bytes(dlc,'big').hex().upper(),
                             'stable_bit_masks':mask.to_bytes(dlc,'big').hex().upper(),
                             'stable_values':(p['first'] & mask).to_bytes(dlc,'big').hex().upper()
                             if can_id <= 0x7FF and can_id != 0x1EF else None,
                             'device_interval_ms_most_common':p['periods'].most_common(5),
                             'device_interval_ms_max':max(p['periods']) if p['periods'] else None})
    candidates = []
    for (index,role,can_id,dlc,byte), counts in sorted(windows.items()):
        totals = [sum(c.values()) for c in counts]
        if min(totals) < 3:
            continue
        for bit in range(8):
            fractions = [sum(n for v,n in c.items() if v & (1 << bit))/total for c,total in zip(counts,totals)]
            if not ((fractions[0] <= .05 and fractions[1] >= .95) or (fractions[0] >= .95 and fractions[1] <= .05)):
                continue
            marker = markers[index]
            known = [s['name'] for m in catalog['messages'] if (m['role'],int(m['can_id'],16))==(role,can_id)
                     for s in m['signals'] if any(b==byte and low <= bit < low+width for b,low,width in s['parts'])]
            nearby = [e['id'] for e in markers if e['id'] != marker['id'] and abs(e['host_ns']-marker['host_ns'])<7_000_000_000]
            candidates.append({'event_id':marker['id'],'label':marker['label'],'role':role,'can_id':f'0x{can_id:03X}',
                               'dlc':dlc,'byte':byte,'bit':bit,'before':int(fractions[0]>=.95),'after':int(fractions[1]>=.95),
                               'samples_before_after':totals,'one_fraction_before_after':fractions,
                               'known_fields':known,'nearby_event_ids':nearby,'status':'exploratory_bit_step'})
    return {'frame_profiles':observations, 'candidate_bit_steps':candidates,
            'candidate_limits':'Uncorrected exploratory comparisons, not discovered functions. Counters, ignition, motion, overlapping actions and human marker delay can produce false matches. Require repeated opposite actions, independent sessions and negative controls.'}
