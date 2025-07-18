import os
import librosa
import soundfile as sf
from tqdm import tqdm

def cut_audio_into_segments(audio_file, output_folder, segment_duration=1.0):
    """
    Cut an audio file into segments of specified duration.
    
    Parameters:
    audio_file (str): Path to the input audio file.
    output_folder (str): Path to the folder where segments will be saved.
    segment_duration (float): Duration of each segment in seconds (default: 1.0).
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_folder, exist_ok=True)
    
    # Load audio file
    print(f"Loading audio file: {audio_file}")
    y, sr = librosa.load(audio_file, sr=None)
    
    # Calculate segment length in samples
    segment_length = int(segment_duration * sr)
    
    # Calculate number of segments
    num_segments = int(len(y) / segment_length)
    
    print(f"Cutting audio into {num_segments} segments of {segment_duration} second(s) each")
    
    # Cut audio into segments and save each segment
    for i in tqdm(range(num_segments)):
        start_sample = i * segment_length
        end_sample = start_sample + segment_length
        
        # Extract segment
        segment = y[start_sample:end_sample]
        
        # Create output file name
        output_file = os.path.join(output_folder, f"segment_{i+1:03d}.wav")
        
        # Save segment
        sf.write(output_file, segment, sr)
    
    print(f"All segments saved to {output_folder}")

if __name__ == "__main__":
    audio_file = "vacuum_cleaner.wav"  # Path to the input audio file
    output_folder = "audio/vacuum_cleaner"   # Output folder name
    segment_duration = 1.0              # Duration of each segment in seconds
    
    cut_audio_into_segments(audio_file, output_folder, segment_duration)