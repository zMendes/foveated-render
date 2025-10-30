import sys
import matplotlib.pyplot as plt
import numpy as np
import pyfvvdp
import torch
import csv
import imageio


def main():
    args = sys.argv[1:]
    reference = args[0]
    foveated = args[1]
    fv = pyfvvdp.fvvdp(display_name='sdr_fhd_24', foveated=True)

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
            fixations.append((x,y))
    reader = imageio.get_reader(f'../output/{reference}.mp4', 'ffmpeg')
    frames = [frame for frame in reader]

    fov_reader = imageio.get_reader(f'../output/{foveated}.mp4', 'ffmpeg')
    fov_frames = [frame for frame in fov_reader]

    n_frames = len(fov_frames)

    if len(fixations) < n_frames:
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

    rows = []
    with open("../output/results.csv", newline='') as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        rows = list(reader)

    for row in rows:
        if row["name"] == foveated:
            row["quality"] = float(Q_JOD.cpu().numpy())
            break
    with open("../output/results.csv", "w", newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    return Q_JOD.cpu().numpy()


if __name__ == "__main__":
    result = main()
    print(result)
