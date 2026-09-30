#include "modbus.h"

void MODBUS::stepPowmrHunt()
{
    if (!_powmrHunt.running) return;
    const uint16_t timeout = _mCom.getResponseTimeout();
    _mCom.setResponseTimeout(500);
    _mCom.clearReadCache();
    auto *mb = _mCom.getModbusMaster();
    vTaskDelay(pdMS_TO_TICKS(10)); // RTU inter-frame quiet interval
    _powmrHunt.step([&](uint8_t function, uint16_t reg, uint16_t &value) {
        const uint8_t result = function == 3 ? mb->readHoldingRegisters(reg, 1)
                                             : mb->readInputRegisters(reg, 1);
        if (result == mb->ku8MBSuccess) value = mb->getResponseBuffer(0);
        return result;
    });
    _mCom.setResponseTimeout(timeout);
    _mCom.clearReadCache();
}

String MODBUS::powmrHuntCommand(String command)
{
    auto &hunt = _powmrHunt;
    if (command == "powmr hunt baseline" || command.startsWith("powmr hunt baseline ")) {
        if (_powmrDumpRunning || hunt.running) return "ERROR: diagnostic already running; use powmr hunt status";
        unsigned long first = 0, last = 65535;
        if (command != "powmr hunt baseline") {
            String args = command.substring(20);
            args.trim();
            // sscanf alone accepts trailing junk and negative unsigned values.
            const int split = args.indexOf(' ');
            if (split < 1) return "ERROR: powmr hunt baseline [first last], addresses 0..65535";
            String a = args.substring(0, split), b = args.substring(split + 1);
            b.trim();
            if (a.isEmpty() || b.isEmpty() || a.length() > 5 || b.length() > 5)
                return "ERROR: invalid range";
            for (unsigned int i = 0; i < a.length(); ++i)
                if (a[i] < '0' || a[i] > '9') return "ERROR: invalid first address";
            for (unsigned int i = 0; i < b.length(); ++i)
                if (b[i] < '0' || b[i] > '9') return "ERROR: invalid last address";
            first = a.toInt(); last = b.toInt();
            if (first > last || last > 65535) return "ERROR: range must be within 0..65535";
        }
        if (!hunt.start(first, last)) return "ERROR: cannot allocate hunt snapshot";
    } else if (command == "powmr hunt compare") {
        if (_powmrDumpRunning || !hunt.compare())
            return "ERROR: finish baseline first; another scan must not be running";
    } else if (command == "powmr hunt cancel") {
        if (hunt.running) hunt.stop();
    } else if (command != "powmr hunt status" && command != "powmr hunt result" &&
               !command.startsWith("powmr hunt result ")) {
        return "READ ONLY: powmr hunt baseline [first last] | compare | status | result [page] | cancel. "
               "Default: FC03 + FC04, all addresses 0..65535, starting at 4000; may take many hours. "
               "4096 readable values maximum; RAM snapshot lost on reboot. Cancel retains partial baseline.";
    }
    String answer = "POWMR_HUNT ";
    answer += hunt.outcome;
    answer += hunt.comparing ? " COMPARE" : " BASELINE";
    answer += " progress=" + String(hunt.cursor) + "/" + String(hunt.total);
    answer += " readable=" + String(hunt.count);
    answer += " rejected=" + String(hunt.rejected) + " transport_errors=" + String(hunt.errors);
    answer += "\nBaseline coverage=" + String(hunt.coverage) + " visited=" +
              String(hunt.baselineVisited) + "/" + String(hunt.baselineTotal) +
              " transport_errors=" + String(hunt.baselineErrors);
    if (hunt.running) return answer;
    if (hunt.comparing) {
        uint32_t changed = 0, unread = 0;
        for (uint32_t i = 0; i < hunt.count; ++i) {
            const auto &e = hunt.entries[i];
            if (e.error) ++unread;
            else if (e.current != e.baseline) ++changed;
        }
        answer += "\nchanged=" + String(changed) + " unread=" + String(unread);
        answer += " (fixed baseline; compare rereads discovered addresses only)";
    }
    if (!command.startsWith("powmr hunt result")) {
        answer += "\nUse powmr hunt result for values. Baseline stays fixed until a new baseline or reboot.";
        return answer;
    }
    uint32_t page = 0;
    if (command != "powmr hunt result") {
        String arg = command.substring(18); arg.trim();
        if (arg.isEmpty() || arg.length() > 3) return "ERROR: invalid page";
        for (unsigned int i = 0; i < arg.length(); ++i)
            if (arg[i] < '0' || arg[i] > '9') return "ERROR: invalid page";
        page = arg.toInt();
    }
    constexpr uint32_t pageSize = 40;
    uint32_t rows = 0;
    for (uint32_t i = 0; i < hunt.count; ++i) {
        const auto &e = hunt.entries[i];
        if (hunt.comparing && !e.error && e.current == e.baseline) continue;
        if (rows / pageSize == page) {
            const uint16_t before = (e.baseline >> 8) | (e.baseline << 8);
            const uint16_t after = (e.current >> 8) | (e.current << 8);
            answer += String("\nFC") + String(e.function) + " reg=" + String(e.reg) +
                      " raw=" + String(e.baseline) + " swap=" + String(before);
            if (hunt.comparing) {
                if (e.error) answer += " READ_ERROR=" + String(e.error);
                else answer += " -> raw=" + String(e.current) + " swap=" + String(after);
            }
        }
        ++rows;
    }
    const uint32_t pages = rows ? (rows + pageSize - 1) / pageSize : 1;
    if (page >= pages) return "ERROR: page out of range; pages=" + String(pages);
    answer += "\npage=" + String(page) + " pages=" + String(pages) + " rows=" + String(rows);
    return answer;
}
