"""Bestilte mål og budsjett for ørkenplantene, fastsatt før bygging. Meter, Z opp, rot i (0, 0, 0)."""

BUDGET = (3000, 900, 200)  # LOD0, LOD1, LOD2: trekanter, øvre grense (strengt under).

# height: tillatt totalhøyde. tuft: tillatt høyde på bladtuen alene (yucca). spread: største bredde.
SPEC = {
    'cholla_a': {'mesh': 'SM_DP_Cholla_A', 'species': 'cholla', 'height': (1.5, 2.0), 'spread': 2.2},
    'cholla_b': {'mesh': 'SM_DP_Cholla_B', 'species': 'cholla', 'height': (1.5, 2.0), 'spread': 2.2},
    'yucca_a': {'mesh': 'SM_DP_Yucca_A', 'species': 'yucca', 'height': (0.6, 1.0), 'spread': 1.8},
    'yucca_b': {'mesh': 'SM_DP_Yucca_B', 'species': 'yucca', 'height': (0.6, 1.0), 'spread': 1.8},
    'yucca_c': {'mesh': 'SM_DP_Yucca_Bloom', 'species': 'yucca', 'height': (2.0, 3.0), 'tuft': (0.6, 1.0), 'spread': 1.8},
    'yucca_d': {'mesh': 'SM_DP_Yucca_DryStalk', 'species': 'yucca', 'height': (2.0, 3.0), 'tuft': (0.6, 1.0), 'spread': 1.8},
    'mesquite_a': {'mesh': 'SM_DP_Mesquite_A', 'species': 'mesquite', 'height': (1.0, 2.5), 'spread': 3.4},
    'mesquite_b': {'mesh': 'SM_DP_Mesquite_B', 'species': 'mesquite', 'height': (1.0, 2.5), 'spread': 3.4},
    'creosote_a': {'mesh': 'SM_DP_Creosote_A', 'species': 'creosote', 'height': (0.8, 1.8), 'spread': 2.2},
    'creosote_b': {'mesh': 'SM_DP_Creosote_B', 'species': 'creosote', 'height': (0.8, 1.8), 'spread': 2.2},
    'creosote_c': {'mesh': 'SM_DP_Creosote_C', 'species': 'creosote', 'height': (0.8, 1.8), 'spread': 2.2},
}

MATERIAL = {'cholla': 'M_DP_Cholla', 'yucca': 'M_DP_Yucca', 'mesquite': 'M_DP_Mesquite', 'creosote': 'M_DP_Creosote'}
SPECIES = ('cholla', 'yucca', 'mesquite', 'creosote')

# Atlasene: (u0, v0, u1, v1), v opp fra bunnen av bildet som i Blender. size er piksler.
ATLAS = {
    'cholla': {
        'size': 1024, 'masked': True,
        'skin': (0.0, 0.0, 0.5, 0.75), 'bark': (0.0, 0.75, 0.5, 1.0), 'spines': (0.5, 0.0, 1.0, 0.5),
        'dry': (0.5, 0.5, 0.75, 1.0), 'fruit': (0.75, 0.5, 1.0, 0.75),
    },
    'yucca': {
        'size': 1024, 'masked': False,
        'green_a': (0.0, 0.0, 0.15, 1.0), 'green_b': (0.15, 0.0, 0.3, 1.0),
        'dead_a': (0.3, 0.0, 0.45, 1.0), 'dead_b': (0.45, 0.0, 0.6, 1.0),
        'stalk': (0.6, 0.0, 0.72, 1.0), 'trunk': (0.72, 0.0, 0.84, 0.5), 'pod': (0.72, 0.5, 0.84, 0.75),
        'flower': (0.84, 0.0, 1.0, 0.33), 'bud': (0.84, 0.33, 1.0, 0.66), 'bract': (0.84, 0.66, 1.0, 1.0),
    },
    'mesquite': {
        'size': 2048, 'masked': True,
        'bark': (0.0, 0.0, 0.125, 1.0), 'twig': (0.125, 0.0, 0.25, 1.0),
        'leaf_a': (0.25, 2 / 3, 0.5, 1.0), 'leaf_b': (0.5, 2 / 3, 0.75, 1.0), 'leaf_c': (0.75, 2 / 3, 1.0, 1.0),
        'bare_a': (0.25, 1 / 3, 0.5, 2 / 3), 'bare_b': (0.5, 1 / 3, 0.75, 2 / 3), 'pod_a': (0.75, 1 / 3, 1.0, 2 / 3),
        'far_a': (0.25, 0.0, 0.5, 1 / 3), 'far_b': (0.5, 0.0, 0.75, 1 / 3), 'far_c': (0.75, 0.0, 1.0, 1 / 3),
    },
    'creosote': {
        'size': 1024, 'masked': True,
        'bark': (0.0, 0.0, 0.125, 1.0), 'twig': (0.125, 0.0, 0.25, 1.0),
        'leaf_a': (0.25, 2 / 3, 0.5, 1.0), 'leaf_b': (0.5, 2 / 3, 0.75, 1.0), 'leaf_c': (0.75, 2 / 3, 1.0, 1.0),
        'tip_a': (0.25, 1 / 3, 0.5, 2 / 3), 'tip_b': (0.5, 1 / 3, 0.75, 2 / 3), 'bare_a': (0.75, 1 / 3, 1.0, 2 / 3),
        'far_a': (0.25, 0.0, 0.5, 1 / 3), 'far_b': (0.5, 0.0, 0.75, 1 / 3), 'far_c': (0.75, 0.0, 1.0, 1 / 3),
    },
}
