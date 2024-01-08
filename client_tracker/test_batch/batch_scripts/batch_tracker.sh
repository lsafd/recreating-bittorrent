#/sbin/ifquery eth0
#echo -e ""

script_dir="$(dirname "$(readlink -f "$0")")"

# Execute ./bin/tracker relative to the script location
"$script_dir/../../bin/tracker"
