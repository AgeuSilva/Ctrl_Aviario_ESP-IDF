# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

set(app_sources
    "main.c"
	"App_Azure.c"
	"App_ControlSaidas.c"
	"App_Display.c"
	"App_SensorDHT.c"
	"App_Supervisorio.c"
	"App_Teclado.c"
	"Lib_Dht11.c"
	"Lib_HD44780.c"
)

idf_component_register(SRCS ${app_sources})
