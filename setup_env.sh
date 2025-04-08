# asked gpt to generate this cuz im not too familiar with env variables.
#!/bin/bash

# Script to set up the Gemini API key as an environment variable

# Check if an API key was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <your_gemini_api_key>"
    echo "Example: $0 askjdnhaksdskajajskd"
    exit 1
fi

API_KEY="$1"

# Add the API key to various config files for persistence
# Choose the method that works best for your system

# 1. Add to current shell session
export GEMINI_API_KEY="$API_KEY"
echo "API key set for current session"

# 2. Add to .bashrc for persistence
if [ -f "$HOME/.bashrc" ]; then
    # Check if it already exists
    if grep -q "GEMINI_API_KEY" "$HOME/.bashrc"; then
        # Update existing entry
        sed -i "s|export GEMINI_API_KEY=.*|export GEMINI_API_KEY=\"$API_KEY\"|" "$HOME/.bashrc"
    else
        # Add new entry
        echo "" >> "$HOME/.bashrc"
        echo "# Gemini API key" >> "$HOME/.bashrc"
        echo "export GEMINI_API_KEY=\"$API_KEY\"" >> "$HOME/.bashrc"
    fi
    echo "API key added to ~/.bashrc"
fi

# 3. Add to .profile for login shells
if [ -f "$HOME/.profile" ]; then
    if grep -q "GEMINI_API_KEY" "$HOME/.profile"; then
        sed -i "s|export GEMINI_API_KEY=.*|export GEMINI_API_KEY=\"$API_KEY\"|" "$HOME/.profile"
    else
        echo "" >> "$HOME/.profile"
        echo "# Gemini API key" >> "$HOME/.profile"
        echo "export GEMINI_API_KEY=\"$API_KEY\"" >> "$HOME/.profile"
    fi
    echo "API key added to ~/.profile"
fi

# Create a local .env file for the project
echo "GEMINI_API_KEY=\"$API_KEY\"" > .env
echo "API key saved to .env file"

echo ""
echo "API key setup complete!"
echo "To use the API key in the current session, run:"
echo "  source ~/.bashrc"
echo "or restart your terminal."
echo ""
echo "You can test if the key is set by running:"
echo "  echo \$GEMINI_API_KEY"