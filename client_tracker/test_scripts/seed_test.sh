IFQUERY_OUT=$(/sbin/ifquery eth0)
IP_ADDR=$(echo "$IFQUERY_OUT" | awk '/address:/ {print $2}')
echo "Client IP: ($IP_ADDR)"

IP_ADDR=130.58.68.194

script_dir="$(dirname "$(readlink -f "$0")")"

"$script_dir"/../bin/client -f "$script_dir"/../test_scripts/test_data/aot.sly -s "$script_dir"/../test_scripts/test_data/
