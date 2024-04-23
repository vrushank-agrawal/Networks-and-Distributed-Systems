# Clean the project
make clean

# Build the project
make

# Run all test files in the tests directory
port=5120

# delete the time.txt file if it exists
rm -f time.txt

for file in tests/*; do
    # Run the receiver and sender in separate terminals
    gnome-terminal -- ./receiver "$port" ./copy
    receiver_pid=$!
    gnome-terminal -- ./sender 127.0.0.1 "$port" "$file"
    sender_pid=$!

    # Get the start time in microseconds
    start_time=$(date +%s%6N)

    # Wait for the receiver and sender to finish
    wait $receiver_pid
    wait $sender_pid

    # Calculate and output the time for each file
    end_time=$(date +%s%6N)
    elapsed_time=$((end_time - start_time))
    echo "File: $file \t $elapsed_time microseconds" >> time.txt

    # Kill the receiver and sender
    pkill -f receiver
    pkill -f sender

    # Compare the original file with the copied file
    diff_result=$(diff "$file" ./copy)
    if [ -z "$diff_result" ]; then
        echo "No difference found in $file"
    else
        echo "Difference found in $file"
        rm ./copy
    fi

    rm ./copy
done
