# create_sim_structure.ps1
# 用途：在当前目录创建电机仿真工程的子目录和空文件
# 使用前提：已在 6.Motor_Ctrl_Sim_Py 目录中运行

$rootDir = "."  # 当前目录

# 创建子目录
$dirs = @(
    "models",
    "controllers",
    "simulation",
    "gui",
    "utils"
)

foreach ($dir in $dirs) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force
        Write-Host "📁 Created directory: $dir" -ForegroundColor Cyan
    }
}

# 创建 __init__.py
$initFiles = @(
    "__init__.py",
    "models/__init__.py",
    "controllers/__init__.py",
    "simulation/__init__.py",
    "gui/__init__.py",
    "utils/__init__.py"
)

foreach ($file in $initFiles) {
    if (!(Test-Path $file)) {
        New-Item -ItemType File -Path $file -Force
        Write-Host "📄 Created __init__.py: $file" -ForegroundColor Yellow
    }
}

# 创建主要 Python 文件和配置文件
$files = @(
    "main.py",
    "models/dc_motor.py",
    "controllers/pid_controller.py",
    "controllers/foc_controller.py",
    "simulation/simulator.py",
    "gui/pid_tuner_gui.py",
    "utils/signal_utils.py",
    "requirements.txt",
    "README.md"
)

foreach ($file in $files) {
    if (!(Test-Path $file)) {
        New-Item -ItemType File -Path $file -Force
        Write-Host "📝 Created file: $file" -ForegroundColor Magenta
    }
}

Write-Host "✅ Motor control simulation project structure initialized!" -ForegroundColor Green
Write-Host "👉 Next: Start editing main.py and module files." -ForegroundColor Blue