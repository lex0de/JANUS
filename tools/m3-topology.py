#!/usr/bin/env python3
# SPDX-License-Identifier: ISC
"""Check the generated capDL graph, independently of JANUS dispatch tests."""
import json
from pathlib import Path


def main():
    if not __debug__:
        raise SystemExit('Run topology assertions without Python optimisation')
    objects = json.loads(Path('out/m3-release/capdl.json').read_text())['objects']
    names = {item['name']: i for i, item in enumerate(objects)}

    def slots(name, kind):
        return objects[names[name]]['object'][kind]['slots']

    def frames(index):
        result = set()
        for slot in objects[index]['object']['PageTable']['slots']:
            for kind, cap in slot['cap'].items():
                if kind == 'Frame':
                    result.add(cap['object'])
                elif kind == 'PageTable':
                    result.update(frames(cap['object']))
        return result

    private = names['frame_mr_object_private_000000000']
    mappings = {}
    for name in ('control', 'object', 'world_a', 'world_b', 'attacker'):
        mapped = frames(names['pml4_' + name])
        assert (private in mapped) == (name == 'object'), name
        mappings[name] = mapped
        print(f'{name}: {len(mapped)} mapped 4-KiB frames; private={private in mapped}')
    for world, badge in [('world_a', (1 << 63) | 1), ('world_b', (1 << 63) | 2)]:
        entries = slots('cnode_' + world, 'CNode')
        endpoints = [slot['cap']['Endpoint'] for slot in entries if 'Endpoint' in slot['cap']]
        service = [cap for cap in endpoints if cap['object'] == names['ep_object']]
        assert len(service) == 1 and service[0]['badge'] == badge
        assert service[0]['rights'] == dict(read=False, write=True, grant=False, grant_reply=True)
        assert all(cap['object'] in (names['ep_object'], names['ep_fault_monitor']) for cap in endpoints)
        assert all(not cap['rights']['read'] and not cap['rights']['grant'] for cap in endpoints)
        assert not any(kind in slot['cap'] for slot in entries for kind in ('Tcb', 'IOPorts', 'Untyped'))
        assert not (mappings[world] & mappings['object'])
    assert not (mappings['world_a'] & mappings['world_b'])
    owner_caps = [slot['cap']['Endpoint'] for slot in slots('cnode_control', 'CNode')
                  if 'Endpoint' in slot['cap'] and slot['cap']['Endpoint']['object'] == names['ep_object']]
    assert len(owner_caps) == 1 and owner_caps[0]['badge'] == 1 << 63
    for name, budget in [('control', 2000), ('object', 1000), ('world_a', 1000), ('world_b', 1000), ('attacker', 1000)]:
        sc = objects[names['sched_context_' + name]]['object']['SchedContext']['extra']
        assert sc['budget'] == budget and sc['period'] == 10000
    print('PASS generated capabilities: distinct badges, no world owner/control/device caps, disjoint memory, budgets')


if __name__ == '__main__':
    main()
