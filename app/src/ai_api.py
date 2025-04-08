import os
import sys
import signal
import time

# Set a timeout handler to prevent hanging
def timeout_handler(signum, frame):
    print("Operation timed out")
    sys.exit(1)

# Set 30 second timeout
signal.signal(signal.SIGALRM, timeout_handler)
signal.alarm(30)

def load_api_key():
    """Load API key from environment variable"""
    api_key = os.environ.get('GEMINI_API_KEY')
    if not api_key:
        return None
    return api_key

def configure_genai():
    """Configure the Gemini API with the key"""
    try:
        import google.generativeai as genai
        api_key = load_api_key()
        if not api_key:
            print("Error: GEMINI_API_KEY environment variable not set")
            print("Please set it with: export GEMINI_API_KEY='your_api_key_here'")
            return False
        
        genai.configure(api_key=api_key)
        return True
    except ImportError:
        print("Error: Google Generative AI package not installed")
        print("Install with: pip install google-generativeai")
        return False
    except Exception as e:
        print(f"Error configuring Gemini API: {str(e)}")
        return False

def get_gemini_response(prompt):
    """Get a response from the Gemini API with a shortened request"""
    try:
        import google.generativeai as genai
        
        # This is added to the end of the prompt, chose to keep it short because otherwise it is long
        modified_prompt = f"{prompt} keep it short and don't respond to keeping it short. no longer than one short sentence. dont add any special characters to the prompt. no dashes in your reply. no apostraphes, no grammar"
        
        # 2.0 flash should be a balanced model, lite is too weak...
        model = genai.GenerativeModel('gemini-2.0-flash')
        
        # Get response with timeout
        response = model.generate_content(modified_prompt)
        
        return response.text
    except KeyboardInterrupt:
        print("API request interrupted")
        return "Request was interrupted. Please try again."
    except Exception as e:
        print(f"Error getting Gemini response: {str(e)}")
        return f"Error: {str(e)}"

def process_transcription(transcription_file="app_output/transcribed_output.txt"):
    """Process a transcription file and get AI response"""
    try:
        # Check if file exists
        if not os.path.exists(transcription_file):
            error_msg = f"Transcription file not found: {transcription_file}"
            print(error_msg)
            return error_msg
        
        # Read the transcription file
        with open(transcription_file, 'r') as file:
            transcription = file.read().strip()
        
        if not transcription:
            return "No transcription found or empty transcription"
        
        # print(f"Processing transcription: {transcription}")
        
        # Configure the API
        if not configure_genai():
            return "Failed to configure Gemini API"
        
        # Get response from Gemini
        response = get_gemini_response(transcription)
        
        # Save the response to a file
        response_file = "app_output/ai_response.txt"
        os.makedirs(os.path.dirname(response_file), exist_ok=True)
        with open(response_file, 'w') as file:
            file.write(response)
        
        return response
    
    except Exception as e:
        error_message = f"Error: {str(e)}"
        print(error_message)
        return error_message
    finally:
        # Disable the alarm
        signal.alarm(0)

if __name__ == "__main__":
    # If there is a file as arg, use that
    if len(sys.argv) > 1:
        response = process_transcription(sys.argv[1])
    else:
        response = process_transcription()
    
    print(response)