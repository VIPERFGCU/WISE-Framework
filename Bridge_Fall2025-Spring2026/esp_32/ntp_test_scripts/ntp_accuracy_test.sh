#!/bin/bash

# Settings
INTERVAL=1      # Interval between polls in seconds
POLLS=600       # Number of times to poll
LOG_FILE="ntpdate.log"

# Create log directory if it doesn't exist
mkdir -p "$(dirname "$LOG_FILE")"

# Loop with seq for proper variable use
for i in $(seq 1 $POLLS); do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$TIMESTAMP] Run #$i" | tee -a "$LOG_FILE"
    
    # Run ntpdate and log both to file and terminal
    ntpdate -q 10.153.124.3 2>&1 | tee -a "$LOG_FILE"

    echo "" | tee -a "$LOG_FILE"

    sleep "$INTERVAL"
done

