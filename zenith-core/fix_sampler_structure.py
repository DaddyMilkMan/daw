import os
import shutil

base_dir = r'C:\zenith\daw\zenith-core\Content\Instruments\ZenithSampler'
patch_dir = os.path.join(base_dir, 'LoFi Keys')
samples_dir = os.path.join(patch_dir, 'Samples')

if not os.path.exists(samples_dir):
    os.makedirs(samples_dir)

# Move .zpatch
src_patch = os.path.join(base_dir, 'LoFi Keys.zpatch')
dst_patch = os.path.join(patch_dir, 'LoFi Keys.zpatch')
if os.path.exists(src_patch):
    shutil.move(src_patch, dst_patch)
    print(f"Moved patch to {dst_patch}")

# Copy samples
src_samples_dir = os.path.join(base_dir, 'Samples')
if os.path.exists(src_samples_dir):
    for f in os.listdir(src_samples_dir):
        if f.endswith('.wav'):
            shutil.copy2(os.path.join(src_samples_dir, f), os.path.join(samples_dir, f))
            print(f"Copied {f}")
