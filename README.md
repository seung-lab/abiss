# Abiss
Affinity based image segmentation system, implementing algorithm described in https://arxiv.org/abs/2106.10795

# Build
```sh
mkdir build && cd build
cmake ..
make
```

# Python interface

Install from local directory
```sh
pip install .
```

Run watershed
```python
# affs is a [width, height, depth, 3] numpy array of float32
affs = ...
segmentations = abiss.watershed(
        affs=affs,
        aff_threshold_low=0.01,
        aff_threshold_high=0.09,
        size_threshold=200,
    )
```
