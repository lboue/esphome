import esphome.codegen as cg
import esphome.config_validation as cv

# from esphome.const import CONF_ID
# from esphome.core import CORE

CODEOWNERS = ["@esphome/core"]

CONF_IMPLEMENTATION = "implementation"
IMPLEMENTATION_LWIP_TCP = "lwip_tcp"
IMPLEMENTATION_LWIP_SOCKETS = "lwip_sockets"
IMPLEMENTATION_BSD_SOCKETS = "bsd_sockets"
IMPLEMENTATION_ZEPHYR_SOCKETS = "zephyr_sockets"

socket_ns = cg.esphome_ns.namespace("socket")
Socket = socket_ns.class_("Socket")

# No need to auto-load zephyr_dns anymore, as it's part of zephyr_openthread
AUTO_LOAD = []

# No need for zephyr_dns dependency anymore
DEPENDENCIES = []

CONFIG_SCHEMA = cv.Schema(
    {
        cv.SplitDefault(
            CONF_IMPLEMENTATION,
            esp8266=IMPLEMENTATION_LWIP_TCP,
            esp32=IMPLEMENTATION_BSD_SOCKETS,
            rp2040=IMPLEMENTATION_LWIP_TCP,
            nrf52=IMPLEMENTATION_ZEPHYR_SOCKETS,
            bk72xx=IMPLEMENTATION_LWIP_SOCKETS,
            rtl87xx=IMPLEMENTATION_LWIP_SOCKETS,
            host=IMPLEMENTATION_BSD_SOCKETS,
        ): cv.one_of(
            IMPLEMENTATION_LWIP_TCP,
            IMPLEMENTATION_LWIP_SOCKETS,
            IMPLEMENTATION_BSD_SOCKETS,
            IMPLEMENTATION_ZEPHYR_SOCKETS,
            lower=True,
            space="_",
        ),
    }
)


async def to_code(config):
    if CONF_IMPLEMENTATION not in config:
        raise ValueError(f"Missing required configuration key: {CONF_IMPLEMENTATION}")

    impl = config[CONF_IMPLEMENTATION]
    if impl == IMPLEMENTATION_LWIP_TCP:
        cg.add_define("USE_SOCKET_IMPL_LWIP_TCP")
    elif impl == IMPLEMENTATION_LWIP_SOCKETS:
        cg.add_define("USE_SOCKET_IMPL_LWIP_SOCKETS")
    elif impl == IMPLEMENTATION_BSD_SOCKETS:
        cg.add_define("USE_SOCKET_IMPL_BSD_SOCKETS")
    elif impl == IMPLEMENTATION_ZEPHYR_SOCKETS:
        cg.add_define("USE_SOCKET_IMPL_ZEPHYR_SOCKETS")

    # Add build flag to enable OpenThread DNS if needed
