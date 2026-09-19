# VelaGesture — CMakeLists.txt

if(CONFIG_VELAGESTURE)
  nuttx_add_application(
    NAME velagesture
    SRCS
      src/main.c
      src/stable_gate.c
      src/gesture_perception.c
      src/ws_client.c
      src/ai_agent.c
      src/skill_executor.c
      src/audio_io.c
    STACKSIZE 8192
    INCLUDE_DIRECTORIES
      include
  )
endif()
