TARGET1 := rootfs_update
TARGET2 := upgrade

all: $(TARGET1) $(TARGET2)

$(TARGET1) : rootfs_update.o img.o env.o upgrade.h boot_head.h dos.h
$(TARGET2) : upgrade.o img.o env.o upgrade.h boot_head.h dos.h
	$(CC) $^ -o $@ -I. -Bstatic

clean:
	rm -rf $(TARGET1) $(TARGET2) *.o
