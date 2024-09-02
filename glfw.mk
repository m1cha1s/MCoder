GLFW_SRC = context.c init.c input.c monitor.c platform.c vulkan.c window.c egl_context.c osmesa_context.c null_init.c null_monitor.c null_window.c null_joystick.c cocoa_time.c posix_module.c posix_thread.c cocoa_init.m cocoa_joystick.m cocoa_monitor.m cocoa_window.m nsgl_context.m

GLFW_OBJ_PRE_M = $(GLFW_SRC:.c=.o)
GLFW_OBJ = $(addprefix build/,$(GLFW_OBJ_PRE_M:.m=.o))

build/glfw.a: glfw $(GLFW_OBJ)
	$(AR) rcs $@ $(GLFW_OBJ)

build/%.o: glfw/src/%.c
	$(CC) -c -o $@ $< -I glfw/src -I glfw/include

build/%.o: glfw/src/%.m
	$(CC) -c -o $@ $< -I glfw/src -I glfw/include

glfw:
	git submodule update --init --recursive
