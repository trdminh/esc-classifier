import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

import librosa
def plot_full_wav(filename):
    sample_rate, data = wavfile.read(filename)

    if data.ndim == 2:
        data = data[:, 0]

    duration = len(data) / sample_rate
    time = np.linspace(0., duration, len(data))

    plt.figure(figsize=(12, 4))
    plt.plot(time, data, linewidth=0.5)
    plt.title(f"Sóng âm của file: {filename}")
    plt.xlabel("Thời gian (giây)")
    plt.ylabel("Biên độ")
    plt.grid(True)
    plt.tight_layout()
    plt.show()

def spectrogram(filename):
    audio = filename
    x , sr = librosa.load(audio)
    X = librosa.stft(x)
    Xdb = librosa.amplitude_to_db(abs(X))
    plt.figure(figsize=(10,5))
    librosa.display.specshow(Xdb, sr=sr, x_axis='time', y_axis='hz')
    plt.colorbar()
    plt.show()

