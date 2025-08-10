import subprocess
import os

# 可配置项
script_dir = os.path.dirname(os.path.abspath(__file__))
test_executable = os.path.abspath(os.path.join(script_dir, "../../build/test/threadpool_test_Tests"))
timeout_seconds = 210
repeat_count = 100

# 统计信息
success_count = 0
failure_count = 0

print(f"Timeout per run: {timeout_seconds}s, Total runs: {repeat_count}")

for i in range(1, repeat_count + 1):
    print(f"\n[Run {i}/{repeat_count}] Running test...")

    try:
        process = subprocess.Popen(test_executable, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate(timeout=timeout_seconds)

        if process.returncode == 0:
            print(f"[Run {i}] Success")
            success_count += 1
        else:
            print(f"[Run {i}] Failed with return code {process.returncode}")

            # 保存 stdout / stderr 到文件
            stdout_file = os.path.join(script_dir, f"run_{i}_stdout.log")
            stderr_file = os.path.join(script_dir, f"run_{i}_stderr.log")
            with open(stdout_file, "wb") as f_out:
                f_out.write(stdout)
            with open(stderr_file, "wb") as f_err:
                f_err.write(stderr)

            print(f"[Run {i}] Logs saved to:\n  {stdout_file}\n  {stderr_file}")

            failure_count += 1

    except subprocess.TimeoutExpired:
        print(f"[Run {i}] Timeout expired after {timeout_seconds}s. Killing process...")
        process.kill()
        # 获取进程在超时前产生的输出
        stdout, stderr = process.communicate()

         # 保存日志
        stdout_file = os.path.join(script_dir, f"run_{i}_timeout_stdout.log")
        stderr_file = os.path.join(script_dir, f"run_{i}_timeout_stderr.log")
        with open(stdout_file, "wb") as f_out:
            f_out.write(stdout)
        with open(stderr_file, "wb") as f_err:
            f_err.write(stderr)

        print(f"[Run {i}] Timeout logs saved to:\n  {stdout_file}\n  {stderr_file}")

        failure_count += 1

print("\n=== Test Summary ===")
print(f"Total runs:    {repeat_count}")
print(f"Success count: {success_count}")
print(f"Failure count: {failure_count}")
