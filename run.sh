#!/bin/bash

# 执行 cmake build
cmake --build build --target check0

# 检查上一个命令的退出状态
while [ $? -ne 0 ]; do
    echo "Build failed. Retrying in 5 seconds..."
    sleep 5
    cmake --build build --target check0
done

# 如果成功退出循环，执行下一步操作
echo "Build succeeded. Continuing with next step..."
# 在这里写下一步操作的命令