if [ ! -f proxy.py ]; then
    echo "Error: proxy.py file not found!"
    exit 1
fi

if [ ! -d tests ]; then
    echo "Error: tests directory not found!"
    exit 1
fi

python3 proxy.py &
pid=$!

sleep 1

python_ports=$(lsof -i -n -P | grep LISTEN | grep python | awk '{print $9}' | cut -d ":" -f2 | sort -u)

for port in $python_ports; do
    echo "Checking port $port..."
    if nc -z localhost "$port"; then
        echo "Proxy is running on port $port"
        proxy_port=$port
        break
    fi
done

if [ -z "$proxy_port" ]; then
    echo "Error: Proxy port not found!"
    exit 1
fi

for test in tests/*.txt; do
    echo "Running $test"
        while IFS= read -r line; do
            echo "$line" | nc localhost "$proxy_port"
        done < "$file"
    fi
done

kill $pid