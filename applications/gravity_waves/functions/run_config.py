"""Makes an executable record the settings it was built and run with.

``embed_run_config`` compiles the given source files and a settings summary into
the solver. When a run starts, the global master writes them to
``<outputPath>/run_config/`` (``-o`` of the executable), together with the
runtime settings (which config.json or the command line may override), the
config.json itself and the generated solver header.
"""

import datetime
import os
import platform
import re
import subprocess

_DELIMITER = "RUNCFG"


def _git(directory, *args):
    try:
        return subprocess.run(
            ["git", "-C", directory, *args], capture_output=True, text=True, check=True
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def _cpp_string(text):
    # The generated code passes through Jinja more than once and the PDE snippets in
    # the sources contain {{...}}: split the raw string literal after every "{" that
    # would open a Jinja tag, the compiler concatenates the pieces again.
    if ")" + _DELIMITER + '"' in text:
        raise ValueError("text cannot be embedded as a raw string literal")
    split = ")" + _DELIMITER + '" R"' + _DELIMITER + "("
    return 'R"' + _DELIMITER + "(" + re.sub(r"\{(?=[{%#])", "{" + split, text) + ")" + _DELIMITER + '"'


def embed_run_config(solver, project_name, source_files, settings):
    """``source_files`` are paths of the scripts that define the run; ``settings``
    maps labels to the values worth reading without opening the scripts."""
    here = os.path.dirname(os.path.abspath(source_files[0]))
    commit = _git(here, "rev-parse", "HEAD") or "unknown"
    if _git(here, "status", "--porcelain", "--untracked-files=no"):
        commit += " (with uncommitted changes)"

    summary = [
        f"built:      {datetime.datetime.now().isoformat(timespec='seconds')} on {platform.node()}",
        f"build cwd:  {os.getcwd()}",
        f"git commit: {commit}",
        "",
    ]
    width = max((len(str(key)) for key in settings), default=0)
    summary += [f"{str(key):<{width}} = {value}" for key, value in settings.items()]

    files = {"build_settings.txt": "\n".join(summary) + "\n"}
    for path in source_files:
        with open(path) as fh:
            files[os.path.basename(path)] = fh.read()

    # exahype2.Project generates into <project_name>.generated below the directory it runs in
    generated_directory = os.path.abspath(project_name + ".generated")
    entries = ",\n    ".join(
        f'{{"{name}", {_cpp_string(content)}}}' for name, content in files.items()
    )

    solver.add_includes("ctime", "filesystem", "fstream", "iomanip", "tarch/mpi/Rank.h")
    solver.add_declarations("static void writeRunConfiguration(const std::string& generatedDirectory);\n")
    solver.add_definitions(
        """
void {{FULL_QUALIFIED_NAMESPACE}}::Abstract{{SOLVER_NAME}}::writeRunConfiguration(const std::string& generatedDirectory) {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }
  const auto& parameters = ::runner::RuntimeParameters::getInstance();
  const std::filesystem::path directory = std::filesystem::path(parameters.getOutputPath()) / "run_config";
  std::error_code error;
  std::filesystem::create_directories(directory, error);

  const std::pair<const char*, const char*> files[] = {
    """
        + entries
        + """
  };
  for (const auto& [name, content] : files) {
    std::ofstream(directory / name) << content;
  }

  const std::time_t now = std::time(nullptr);
  std::ofstream runtime(directory / "runtime_settings.txt");
  runtime << "started:        " << std::put_time(std::localtime(&now), "%Y-%m-%dT%H:%M:%S") << "\\n"
          << "output path:    " << parameters.getOutputPath() << "\\n"
          << "config file:    " << (parameters.getConfigFilePath().empty() ? "none" : parameters.getConfigFilePath()) << "\\n"
          << "terminal time:  " << parameters.getMinTerminalTime() << " .. " << parameters.getMaxTerminalTime() << "\\n"
          << "max time steps: " << (parameters.getNumberOfTimeSteps() > 0 ? std::to_string(parameters.getNumberOfTimeSteps()) : "unbounded") << "\\n"
          << "grid levels:    " << parameters.getGridLevels() << " .. " << parameters.getMaxGridLevels() << "\\n"
          << "plot snapshots: " << parameters.getNumberOfSnapshots() << " (first at t = " << parameters.getFirstPlotTimeStamp() << ")\\n"
          << "MPI ranks:      " << tarch::mpi::Rank::getInstance().getNumberOfRanks() << "\\n";

  if (not parameters.getConfigFilePath().empty()) {
    std::filesystem::copy_file(parameters.getConfigFilePath(), directory / "config.json",
                               std::filesystem::copy_options::overwrite_existing, error);
  }
  // Copied when the run starts, so it matches the binary only if nobody regenerated since the build.
  std::filesystem::copy_file(std::filesystem::path(generatedDirectory) / "{{SOLVER_NAME}}.h",
                             directory / "{{SOLVER_NAME}}.h", std::filesystem::copy_options::overwrite_existing, error);
}
"""
    )
    solver.on_start_simulation(f'writeRunConfiguration({_cpp_string(generated_directory)});\n')
