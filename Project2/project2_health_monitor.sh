#!/bin/bash


LOG_FILE="/var/log/server_health.log"
MONITOR_INTERVAL=60       
MONITORING_PID_FILE="/tmp/server_health_monitor.pid"

# Default thresholds (%)
CPU_THRESHOLD=80
MEM_THRESHOLD=80
DISK_THRESHOLD=90

# Colours 
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# Dependency Check 
check_dependencies() {
  local missing=0
  for cmd in top free df ps awk grep; do
    if ! command -v "$cmd" &>/dev/null; then
      echo -e "${RED}[ERROR] Required command '$cmd' not found.${RESET}" >&2
      missing=1
    fi
  done
  [[ $missing -eq 1 ]] && { echo "Please install missing tools and retry."; exit 1; }
}

# Logging 
log() {
  local level="$1"
  local message="$2"
  local timestamp
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  echo "[$timestamp] [$level] $message" >> "$LOG_FILE"
}

# Ensure log file is writable
init_log() {
  if [[ ! -f "$LOG_FILE" ]]; then
    touch "$LOG_FILE" 2>/dev/null || { LOG_FILE="/tmp/server_health.log"; touch "$LOG_FILE"; }
  fi
  if [[ ! -w "$LOG_FILE" ]]; then
    LOG_FILE="/tmp/server_health.log"
    echo -e "${YELLOW}[WARN] Using fallback log: $LOG_FILE${RESET}"
  fi
}



# Returns CPU usage as integer percentage
get_cpu_usage() {
  # Use /proc/stat for accuracy
  if [[ -r /proc/stat ]]; then
    local cpu_line1 cpu_line2
    cpu_line1=$(grep '^cpu ' /proc/stat)
    sleep 0.5
    cpu_line2=$(grep '^cpu ' /proc/stat)

    local idle1 total1 idle2 total2
    idle1=$(echo "$cpu_line1" | awk '{print $5}')
    total1=$(echo "$cpu_line1" | awk '{for(i=2;i<=NF;i++) s+=$i; print s}')
    idle2=$(echo "$cpu_line2" | awk '{print $5}')
    total2=$(echo "$cpu_line2" | awk '{for(i=2;i<=NF;i++) s+=$i; print s}')

    local idle_diff total_diff
    idle_diff=$((idle2 - idle1))
    total_diff=$((total2 - total1))

    if [[ $total_diff -eq 0 ]]; then
      echo 0
    else
      awk "BEGIN { printf \"%d\", (1 - $idle_diff/$total_diff) * 100 }"
    fi
  else
    top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1 | cut -d'.' -f1
  fi
}

# Returns memory usage percentage
get_mem_usage() {
  free | awk '/^Mem:/ {printf "%d", ($3/$2)*100}'
}

# Returns disk usage percentage for /
get_disk_usage() {
  df / | awk 'NR==2 {print $5}' | tr -d '%'
}

# Returns number of running processes
get_process_count() {
  ps aux --no-headers 2>/dev/null | wc -l
}

#  Display Functions 

# Colour-code a percentage value
colorize() {
  local value=$1
  local threshold=$2
  if   [[ $value -ge $threshold ]];            then echo -e "${RED}${value}%${RESET}"
  elif [[ $value -ge $((threshold * 75/100)) ]]; then echo -e "${YELLOW}${value}%${RESET}"
  else                                               echo -e "${GREEN}${value}%${RESET}"
  fi
}

display_health() {
  local cpu mem disk procs
  
  echo -e "${BOLD}${CYAN}     SERVER HEALTH DASHBOARD${RESET}"
  echo -e "${BOLD}${CYAN}  $(date '+%Y-%m-%d %H:%M:%S')${RESET}"
 

  echo -e "\n${BOLD}Collecting metrics...${RESET}"
  cpu=$(get_cpu_usage)
  mem=$(get_mem_usage)
  disk=$(get_disk_usage)
  procs=$(get_process_count)

  # Validate numeric values
  [[ ! "$cpu"   =~ ^[0-9]+$ ]] && cpu=0
  [[ ! "$mem"   =~ ^[0-9]+$ ]] && mem=0
  [[ ! "$disk"  =~ ^[0-9]+$ ]] && disk=0

  printf "  %-25s %s\n" "CPU Usage:"      "$(colorize $cpu  $CPU_THRESHOLD)"
  printf "  %-25s %s\n" "Memory Usage:"   "$(colorize $mem  $MEM_THRESHOLD)"
  printf "  %-25s %s\n" "Disk Usage (/):" "$(colorize $disk $DISK_THRESHOLD)"
  printf "  %-25s ${CYAN}%s${RESET}\n" "Active Processes:" "$procs"

  echo -e "${BOLD}${CYAN}========================================${RESET}"

  # Log the snapshot
  log "INFO" "CPU=${cpu}% MEM=${mem}% DISK=${disk}% PROCS=${procs}"

  # Threshold alerts
  check_and_alert "CPU"    $cpu  $CPU_THRESHOLD
  check_and_alert "MEMORY" $mem  $MEM_THRESHOLD
  check_and_alert "DISK"   $disk $DISK_THRESHOLD
}

#Alert Logic 
check_and_alert() {
  local resource="$1"
  local value=$2
  local threshold=$3

  if [[ $value -ge $threshold ]]; then
    local msg="ALERT: $resource usage is ${value}% (threshold: ${threshold}%)"
    echo -e "\n  ${RED}⚠  $msg${RESET}"
    log "ALERT" "$msg"
  fi
}

# Threshold Configuration 
configure_thresholds() {
  echo -e "\n${BOLD}Configure Alert Thresholds${RESET}"
  echo -e "Current: CPU=${CPU_THRESHOLD}%  MEM=${MEM_THRESHOLD}%  DISK=${DISK_THRESHOLD}%\n"

  read_percent() {
    local name="$1"
    local current="$2"
    local input
    while true; do
      read -rp "  Enter new $name threshold (1-100) [${current}]: " input
      input="${input:-$current}"
      if [[ "$input" =~ ^[0-9]+$ ]] && (( input >= 1 && input <= 100 )); then
        echo "$input"
        return
      fi
      echo -e "  ${RED}Invalid value. Enter a number between 1 and 100.${RESET}"
    done
  }

  CPU_THRESHOLD=$(read_percent "CPU"    $CPU_THRESHOLD)
  MEM_THRESHOLD=$(read_percent "Memory" $MEM_THRESHOLD)
  DISK_THRESHOLD=$(read_percent "Disk"  $DISK_THRESHOLD)

  echo -e "\n  ${GREEN}Thresholds updated:${RESET} CPU=${CPU_THRESHOLD}%  MEM=${MEM_THRESHOLD}%  DISK=${DISK_THRESHOLD}%"
  log "CONFIG" "Thresholds set: CPU=${CPU_THRESHOLD}% MEM=${MEM_THRESHOLD}% DISK=${DISK_THRESHOLD}%"
}

# Log Management 
view_logs() {
  echo -e "\n${BOLD}Activity Log: $LOG_FILE${RESET}"
  echo -e "${CYAN}$(printf '─%.0s' {1..60})${RESET}"
  if [[ -s "$LOG_FILE" ]]; then
    tail -40 "$LOG_FILE"
  else
    echo "  (Log file is empty)"
  fi
  echo -e "${CYAN}$(printf '─%.0s' {1..60})${RESET}"
}

clear_logs() {
  read -rp "Clear all logs? This cannot be undone. [y/N]: " confirm
  if [[ "$confirm" =~ ^[Yy]$ ]]; then
    > "$LOG_FILE"
    echo -e "  ${GREEN}Log file cleared.${RESET}"
    log "INFO" "Log file cleared by user."
  else
    echo "  Cancelled."
  fi
}

#Background Monitoring 
start_monitoring() {
  if [[ -f "$MONITORING_PID_FILE" ]]; then
    local old_pid
    old_pid=$(cat "$MONITORING_PID_FILE")
    if kill -0 "$old_pid" 2>/dev/null; then
      echo -e "  ${YELLOW}Monitoring is already running (PID $old_pid).${RESET}"
      return
    fi
  fi

  (
    log "INFO" "Background monitoring started (interval: ${MONITOR_INTERVAL}s)"
    while true; do
      cpu=$(get_cpu_usage)
      mem=$(get_mem_usage)
      disk=$(get_disk_usage)
      procs=$(get_process_count)
      log "INFO" "AUTO CPU=${cpu}% MEM=${mem}% DISK=${disk}% PROCS=${procs}"
      check_and_alert "CPU"    "$cpu"  "$CPU_THRESHOLD"  2>/dev/null
      check_and_alert "MEMORY" "$mem"  "$MEM_THRESHOLD"  2>/dev/null
      check_and_alert "DISK"   "$disk" "$DISK_THRESHOLD" 2>/dev/null
      sleep "$MONITOR_INTERVAL"
    done
  ) &

  echo $! > "$MONITORING_PID_FILE"
  echo -e "  ${GREEN}Background monitoring started (PID $!, every ${MONITOR_INTERVAL}s).${RESET}"
}

stop_monitoring() {
  if [[ -f "$MONITORING_PID_FILE" ]]; then
    local pid
    pid=$(cat "$MONITORING_PID_FILE")
    if kill -0 "$pid" 2>/dev/null; then
      kill "$pid"
      rm -f "$MONITORING_PID_FILE"
      echo -e "  ${GREEN}Monitoring stopped (PID $pid).${RESET}"
      log "INFO" "Background monitoring stopped."
    else
      echo -e "  ${YELLOW}No active monitoring process found.${RESET}"
      rm -f "$MONITORING_PID_FILE"
    fi
  else
    echo -e "  ${YELLOW}Monitoring is not currently running.${RESET}"
  fi
}

# sInteractive Menu 
show_menu() {
  echo -e "\n${BOLD}${CYAN}=== Server Health Monitor ===${RESET}"
  echo "  1) Display current system health"
  echo "  2) Configure monitoring thresholds"
  echo "  3) View activity logs"
  echo "  4) Clear logs"
  echo "  5) Start background monitoring"
  echo "  6) Stop background monitoring"
  echo "  7) Exit"
  echo -e "${CYAN}$(printf '─%.0s' {1..32})${RESET}"
  printf "  Select an option [1-7]: "
}

main() {
  check_dependencies
  init_log
  log "INFO" "Script started by $(whoami)"

  while true; do
    show_menu
    read -r choice

    case "$choice" in
      1) display_health ;;
      2) configure_thresholds ;;
      3) view_logs ;;
      4) clear_logs ;;
      5) start_monitoring ;;
      6) stop_monitoring ;;
      7)
        stop_monitoring &>/dev/null
        echo -e "\n${GREEN}Goodbye!${RESET}"
        log "INFO" "Script exited by user."
        exit 0
        ;;
      *)
        echo -e "  ${RED}Invalid option. Please enter a number from 1 to 7.${RESET}"
        ;;
    esac
  done
}

main
