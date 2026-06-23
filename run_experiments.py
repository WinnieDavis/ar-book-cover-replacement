import subprocess
import os
import csv

EXE_PATH = r"./build/objectReplacement.exe"
BASE = r"./TestVideoAndImages"

VIDEO_SETS = [
    (BASE + "/lowQual_720_test.mp4",   BASE + "/grey_book.jpg", BASE + "/red_book.jpg", "720p")
    #(BASE + "/BPlain.mp4",     BASE + "/grey_book.jpg", BASE + "/red_book.jpg", "1080p"),
    #(BASE + "/HighQual_2160_test.mp4", BASE + "/grey_book.jpg", BASE + "/red_book.jpg", "2160p"),
  #  (BASE + "/BLighting.mp4", BASE + "/grey_book.jpg", BASE + "/red_book.jpg", "LightScene"),
  #  (BASE + "/CBin_1080.mp4",      BASE + "/grey_book.jpg", BASE + "/red_book.jpg", "BusyScene"),
]

BASE_RATIO   = 0.75
BASE_RANSAC  = 3.0
BASE_INLIERS = 10

RATIO_RANGE   = [0.6, 0.70, 0.80, 0.90]
RANSAC_RANGE  = [0.3, 1.0, 3.0, 5.0, 10.0]
INLIERS_RANGE = [4, 8, 10, 15, 20, 30]

os.makedirs("results", exist_ok=True)

results = []

def run(video, target, replacement, ratio, ransac, inliers):
    cmd = [EXE_PATH, video, target, replacement, str(ratio), str(ransac), str(inliers)]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    return result.stdout

def parse(output, vid_label, ratio, ransac, inliers):
    for line in output.splitlines():
        if line.startswith("CSV,"):
            parts = line.split(",")
            if len(parts) == 6:
                results.append({
                    "video":        vid_label,
                    "ratio":        ratio,
                    "ransac":       ransac,
                    "min_inliers":  inliers,
                    "frame":        parts[1],
                    "time_s":       parts[2],
                    "good_matches": parts[3],
                    "inlier_count": parts[4],
                    "replaced":     parts[5],
                })

for (video, target, replacement, vid_label) in VIDEO_SETS:
    print(f"\nRunning {vid_label}...")

    for ratio in RATIO_RANGE:
        print(f"  ratio={ratio}")
        output = run(video, target, replacement, ratio, BASE_RANSAC, BASE_INLIERS)
        parse(output, vid_label, ratio, BASE_RANSAC, BASE_INLIERS)

    for ransac in RANSAC_RANGE:
        print(f"  ransac={ransac}")
        output = run(video, target, replacement, BASE_RATIO, ransac, BASE_INLIERS)
        parse(output, vid_label, BASE_RATIO, ransac, BASE_INLIERS)

    for inliers in INLIERS_RANGE:
        print(f"  inliers={inliers}")
        output = run(video, target, replacement, BASE_RATIO, BASE_RANSAC, inliers)
        parse(output, vid_label, BASE_RATIO, BASE_RANSAC, inliers)

with open("results/summaryX1.csv", "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=results[0].keys())
    writer.writeheader()
    writer.writerows(results)

print("\nDone. Results saved to results/summary.csv")