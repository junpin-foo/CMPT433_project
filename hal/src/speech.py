# https://pypi.org/project/SpeechRecognition/
#https://github.com/Uberi/speech_recognition/blob/master/examples/microphone_recognition.py

#!/usr/bin/env python3
import sys
import os
import argparse
import speech_recognition as sr

def check_audio_file(audio_file, verbose=False):
    """
    Check if the audio file is valid
    
    Parameters:
    audio_file (str): Path to the audio file
    verbose (bool): Whether to print verbose diagnostics
    
    Returns:
    bool: True if the file appears valid, False otherwise
    """
    if not os.path.exists(audio_file):
        print(f"Error: File {audio_file} does not exist")
        return False
    
    file_size = os.path.getsize(audio_file)
    if file_size < 1000:  # Less than 1KB
        print(f"Warning: File is very small ({file_size} bytes)")
        return False
    
    if verbose:
        print(f"Audio file: {audio_file}")
        print(f"File size: {file_size} bytes")
    
    return True

def transcribe_audio(audio_file, verbose=False):
    """
    Transcribe using Google's Speech Recognition API
    
    Parameters:
    audio_file (str): Path to the WAV file to transcribe
    verbose (bool): Whether to print verbose diagnostics
    
    Returns:
    str: Transcribed text or error message
    """
    # First check if the file is valid
    if not check_audio_file(audio_file, verbose):
        return "Could not process audio file - file appears invalid"
    
    # Initialize recognizer
    r = sr.Recognizer()
    
    # Set energy threshold for detecting speech
    # Increase = help with sensitivity
    r.energy_threshold = 300
    
    if verbose:
        print("Starting speech recognition process...")
    
    try:
        # Load the audio file
        with sr.AudioFile(audio_file) as source:
            # Adjust for ambient noise
            if verbose:
                print("Adjusting for ambient noise...")
            r.adjust_for_ambient_noise(source, duration=0.5)
            
            # Read the audio data
            if verbose:
                print("Reading audio data...")
            audio_data = r.record(source)
            
            if verbose:
                print("Recognizing speech...")
            
            try:
                # Try with Google Speech Recognition
                text = r.recognize_google(audio_data)
                if verbose:
                    print(f"Google Speech Recognition result: {text}")
                return text
            except sr.UnknownValueError:
                if verbose:
                    print("Google Speech Recognition could not understand audio")
                # You might want to try with higher sensitivity
                try:
                    if verbose:
                        print("Trying again with higher sensitivity...")
                    # Increase sensitivity by lowering the energy threshold
                    r.energy_threshold = 50
                    audio_data = r.record(source)
                    text = r.recognize_google(audio_data)
                    if verbose:
                        print(f"Second attempt result: {text}")
                    return text
                except:
                    return "Could not understand audio"
            except sr.RequestError as e:
                return f"Error with the speech recognition service; {e}"
    except Exception as e:
        return f"Error processing audio file: {str(e)}"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Transcribe WAV audio file to text')
    parser.add_argument('audio_file', help='Path to WAV audio file')
    parser.add_argument('--verbose', '-v', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    # Transcribe and print the result
    result = transcribe_audio(args.audio_file, verbose=args.verbose)
    print(result)