.PHONY: all clean

include config.mk

# 用shell命令@for循环在子文件夹中执行make命令
# make -C dir = make --directory = dir 是在执行dir文件夹中执行make命令
all:
	@for dir in $(BUILD_DIR); \
		do \
			make -C $$dir; \
		done

# 删除make自动生成的文件
clean:
	rm -rf app/link_obj app/dep nginx
	rm -rf signal/*.gch app/*.gch proc/*.gch
