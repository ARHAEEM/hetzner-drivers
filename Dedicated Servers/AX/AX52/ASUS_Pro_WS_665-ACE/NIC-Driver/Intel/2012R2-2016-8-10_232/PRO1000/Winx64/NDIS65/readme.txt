The driver package files in this folder can be used to install drivers for Intel(R) Ethernet Gigabit Adapters on the following operating systems:
  * Microsoft* Windows* 10
  * Microsoft* Windows Server* 2016

The driver package supports devices based on the following controllers:
  * Intel(R) PRO/1000 Network Controller
  * Intel(R) 8256x Gigabit Ethernet Controller
  * Intel(R) 8257x Gigabit Ethernet Controller
  * Intel(R) I210 Gigabit Ethernet Controller
  * Intel(R) I217 Gigabit Ethernet Controller
  * Intel(R) I218 Gigabit Ethernet Controller
  * Intel(R) I219 Gigabit Ethernet Controller
  * Intel(R) I350 Gigabit Ethernet Controller
  * Intel(R) I354 Gigabit Ethernet Controller

NDIS 6.2 introduced new RSS data structures and interfaces. Because of this, you cannot enable RSS on teams that contain a mix of adapters that support NDIS 6.2 RSS and adapters that do not. The e1e6232 driver does not support NDIS 6.2 RSS. If you team one of these devices with a device supported by another driver, the operating system will warn you about the RSS incompatibility. This applies to the following devices:
  * Intel(R) PRO/1000 EB and EB1 Network Connections
  * Intel(R) PRO/1000 PF Network Connections and Adapters
  * Intel(R) PRO/1000 PT Network Connections and Adapters
  * Intel(R) Gigabit PT Quad Port Server ExpressModule
  * Network Connections based on the Intel(R) 82566 Controller
  * Intel(R) PRO/1000 PB Dual Port Server Connection
  * Intel(R) PRO/1000 PB Server Connection
