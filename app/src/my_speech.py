# https://pypi.org/project/SpeechRecognition/
#https://github.com/Uberi/speech_recognition/blob/master/examples/microphone_recognition.py

#!/usr/bin/env python3
import sys
import os
import argparse
import speech_recognition as sr


def transcribe_audio(audio_file):
    """Transcribe using Google's Speech Recognition API"""
    
    # Initialize recognizer
    r = sr.Recognizer()
    
    # Set energy threshold for detecting speech
    # Increase = help with sensitivity
    r.energy_threshold = 300

    
    try:
        # Load the audio file
        with sr.AudioFile(audio_file) as source:
            # Adjust for ambient noise
            r.adjust_for_ambient_noise(source, duration=0.5)
            
            # Read the audio data
            audio_data = r.record(source)

            try:
                # This uses the google speech recognition api (really good)
                text = r.recognize_google(audio_data)
                return text
            except sr.UnknownValueError:
                try:
                    # Increase sensitivity by lowering the energy threshold
                    r.energy_threshold = 50
                    audio_data = r.record(source)
                    text = r.recognize_google(audio_data)
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
    
    args = parser.parse_args()
    
    # Transcribe and print the result
    result = transcribe_audio(args.audio_file)
    print(result)