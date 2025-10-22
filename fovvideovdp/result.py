import matplotlib.pyplot as plt
import numpy as np
import pyfvvdp
import torch
import csv
import imageio


def main():
    fv = pyfvvdp.fvvdp(display_name='standard_fhd', foveated=True)

    # read true gaze position
    fixations = []
    with open("../output/records.csv") as file:
        reader = csv.DictReader(file)
        for row in reader:
            try:
                x = float(row["xT"])
                y = float(row["yT"])
            except KeyError:
                raise KeyError("CSV must have columns named xT and yT")
            fixations.append((x, y))


    reader = imageio.get_reader('../output/ref.mp4', 'ffmpeg')
    frames = [frame for frame in reader]  # list of HxWxC arrays


    fov_reader = imageio.get_reader('../output/video.mp4', 'ffmpeg')
    fov_frames = [frame for frame in fov_reader]  # list of HxWxC arrays

    n_frames = len(fov_frames)

    if len(fixations) < n_frames:
        print(f"Warning: CSV has {len(fixations)} fixations but video has {n_frames} frames; "
            f"Removing initial frames.")
        frames = frames[n_frames - len(fixations):]
        fov_frames = fov_frames[n_frames - len(fixations):]
    elif len(fixations) > n_frames:
        fixations = fixations[:n_frames]


    V_ref = np.stack(frames, axis=3)      # shape H,W,C,N
    V_fov = np.stack(fov_frames, axis=3)      # shape H,W,C,N

    fixation = torch.tensor(fixations, dtype=torch.float32)   # shape [N,2]


    # Gaussian noise with variance 0.003
    Q_JOD, stats = fv.predict(
        V_ref, V_fov, dim_order="HWCF", fixation_point=fixation, frames_per_second=60)

    return Q_JOD

if __name__=="__main__":
    result = main()
    print(result)
