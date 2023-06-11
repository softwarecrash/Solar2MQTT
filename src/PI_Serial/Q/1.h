// 01 01 00 034 030 029 033 00 00 000 0050 2480 13
// 
/*
            [1, "Time until the end of absorb charging", "int", "sec"],
            [2, "Time until the end of float charging", "int", "sec"],
            [
                3,
                "SCC Flag",
                "option",
                ["SCC not communicating?", "SCC is powered and communicating"],
            ],
            [4, "AllowSccOnFlag", "bytes.decode", ""],
            [5, "ChargeAverageCurrent", "bytes.decode", ""],
            [6, "SCC PWM temperature", "int", "°C", {"device-class": "temperature"}],
            [7, "Inverter temperature", "int", "°C", {"device-class": "temperature"}],
            [8, "Battery temperature", "int", "°C", {"device-class": "temperature"}],
            [9, "Transformer temperature", "int", "°C", {"device-class": "temperature"}],
            [10, "GPIO13", "int", ""],
            [11, "Fan lock status", "option", ["Not locked", "Locked"]],
            [12, "Not used", "bytes.decode", ""],
            [13, "Fan PWM speed", "int", "%"],
            [14, "SCC charge power", "int", "W", {"icon": "mdi:solar-power", "device-class": "power"}],
            [15, "Parallel Warning", "bytes.decode", ""],
            [16, "Sync frequency", "float", ""],
            [
                17,
                "Inverter charge status",
                "str_keyed",
                {"10": "nocharging", "11": "bulk stage", "12": "absorb", "13": "float"},
                {"icon": "mdi:book-open"},
            ],
        ],
        "test_responses": [
            b"(00000 00000 01 01 00 059 045 053 068 00 00 000 0040 0580 0000 50.00 13\x39\xB9\r",
*/