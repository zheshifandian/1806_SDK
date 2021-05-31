#!/usr/bin/env python3

from os import getenv
from pathlib import Path
from shutil import rmtree
from subprocess import run
from subprocess import call
import time
import sys
import yaml

profile_folder = Path(getenv("PROFILES", "./profiles")).absolute()


def die(msg: str):
    """Quit script with error message

    msg (str): Error message to print
    """
    print('\033[1;31m' + msg + '\033[0m')
    sys.exit(1)

def warning(msg: str):
    """Quit script with warning message

    msg (str): Error message to print
    """
    print('\033[1;33m' + msg + '\033[0m')


def usage(code: int = 0):
    """Print script usage

    Args:
        code (int): exit code
    """
    print(f"""Usage: {sys.argv[0]} <profile> [options...]

    clean           Remove feeds before setup
    list            List available profiles
    help            Print this message
    """)
    sys.exit(code)

def load_metadata():
    try:
      with open("gl_metadata.yaml", "r") as stream:
        metadata=yaml.safe_load(stream)
        version = metadata["version"]
        call("echo %s > %s" % (version, "files/etc/glversion"), shell=True)
        call("echo %s > %s" % (version, "release"), shell=True)
        version_type = metadata["type"]
        call("echo %s > %s" % (version_type, "files/etc/version.type"), shell=True)
        print("firmware version: " +version)
        print("firmware type: " +version_type)
    except:
      pass
        #print("load gl metadata failed")
    

def load_yaml(fname: str, profile: dict):
    profile_file = (profile_folder / fname).with_suffix(".yml")

    if not profile_file.is_file():
        die(f"Profile {fname} not found")

    new = yaml.safe_load(profile_file.read_text())
    for n in new:
        if n in {"profile", "target", "subtarget", "external_target"}:
            if profile.get(n):
                die(f"Duplicate tag found {n}")
            profile.update({n: new.get(n)})
        elif n in {"description"}:
            profile["description"].append(new.get(n))
        elif n in {"packages"}:
            profile["packages"].extend(new.get(n))
        elif n in {"diffconfig"}:
            profile["diffconfig"] += new.get(n)
        elif n in {"metadata"}:
            profile["metadata"] = new.get(n)
        elif n in {"feeds"}:
            for f in new.get(n):
                if f.get("name", "") == "" or (f.get("uri", "") == "" and f.get("path", "") == ""):
                    die(f"Found bad feed {f}")
                profile["feeds"][f.get("name")] = f
    return profile


def clean_tree():
    print("Cleaning tree")
    rmtree("./tmp", ignore_errors=True)
    rmtree("./packages/feeds/", ignore_errors=True)
    rmtree("./tmp/", ignore_errors=True)
    rmtree(".git/rebase-apply/", ignore_errors=True)
    if Path("./feeds.conf").is_file():
        Path("./feeds.conf").unlink()
    if Path("./.config").is_file():
        Path("./.config").unlink()


def merge_profiles(profiles):
    profile = {"packages": [], "description": [], "metadata":{}, "diffconfig": "", "feeds": {}}

    for p in profiles:
        profile = load_yaml(p, profile)

    return profile


def setup_feeds(profile):
    feeds_conf = Path("feeds.conf")
    if feeds_conf.is_file():
        feeds_conf.unlink()

    feeds = []
    for p in profile.get("feeds", []):
        try:
            f = profile["feeds"].get(p)
            if all(k in f for k in ("branch", "revision", "path")):
                die(f"Please specify either a branch, a revision or a path: {f}")
            if "path" in f:
                feeds.append(f'{f.get("method", "src-link")},{f["name"]},{f["path"]}')
            elif "revision" in f:
                feeds.append(f'{f.get("method", "src-git")},{f["name"]},{f["uri"]}^{f.get("revision")}')
            else:
                feeds.append(f'{f.get("method", "src-git")},{f["name"]},{f["uri"]};{f.get("branch", "master")}')

        except:
            print(f"Badly configured feed: {f}")

    if run(["./scripts/feeds", "uninstall", "-a"]).returncode:
        die(f"Error uninstall feeds")

    if run(["./scripts/feeds", "setup", "-b", *feeds]).returncode:
        die(f"Error setting up feeds")

    if run(["./scripts/feeds", "update"]).returncode:
        warning(f"Error updating feeds")

    print("Install all feeds...")
    #lte_data_oss这个feeds与我们自己的ui包有冲突，先注释掉，后续有问题可以打开
    call("sed -i 's/^src-link lte_data_oss/#src-link lte_data_oss/' ./feeds.conf.default",shell=True)
    if run(["./scripts/feeds", "install", "-a", "-f"]).returncode:
        die(f"Error installing")
    #for p in profile.get("feeds", []):
    #    f = profile["feeds"].get(p)
    #    if run(["./scripts/feeds", "install", "-a", "-f", "-p", f.get("name")]).returncode:
    #        die(f"Error installing {feed}")

    packages = ["./scripts/feeds", "install"]
    for package in profile.get("packages", []):
        packages.append(package)
    if len(packages) > 2:
        if run(packages).returncode:
            die(f"Error installing packages")

    if profile.get("external_target", False):
        if run(["./scripts/feeds", "install", profile["target"]]).returncode:
            die(f"Error installing external target {profile['target']}")

def setup_all_feeds():
    print("Install all feeds...")
    if run(["./scripts/feeds", "install", "-a", "-f"]).returncode:
        die(f"Error installing")

def generate_config(profile):
    config_output = f"""CONFIG_TARGET_{profile["target"]}=y
CONFIG_TARGET_{profile["target"]}_{profile["subtarget"]}=y
CONFIG_TARGET_{profile["target"]}_{profile["subtarget"]}_DEVICE_{profile["profile"]}=y
CONFIG_TARGET_{profile["target"]}_{profile["subtarget"]}_{profile["profile"]}=y
"""

    config_output += f"{profile.get('diffconfig', '')}"

    for package in profile.get("packages", []):
        print(f"Add package to .config: {package}")
        config_output += f"CONFIG_PACKAGE_{package}=y\n"

    Path(".config").write_text(config_output)
    print("Configuration written to .config")


def generate_files(profile):
    if run(["mkdir", "-p", "files/etc"]).returncode:
        die(f"Error create files")

    compile_time = time.strftime('%Y-%m-%d %k:%M:%S', time.localtime(time.time()))
    call("echo %s > %s" % (compile_time, "files/etc/version.date"), shell=True)
    load_metadata()


if __name__ == "__main__":
    if "list" in sys.argv:
        print(f"Profiles in {profile_folder}")
        print("Target Profiles:")
        print("\n".join(sorted(map(lambda p: str(p.stem), profile_folder.glob("target_*.yml")))))
        print("\nFunction Profiles:")
        print("\n".join(sorted(map(lambda p: str(p.stem), profile_folder.glob('[!t]*.yml')))))
        quit(0)

    if "help" in sys.argv:
        usage()

    if len(sys.argv) < 2:
        usage(1)

    if "clean" in sys.argv:
        clean_tree()
        print("Tree is now clean")
        quit(0)

    profile = merge_profiles(sys.argv[1:])

    print("Using the following profiles:")
    for d in profile.get("description"):
        print(f" - {d}")

    clean_tree()
    setup_feeds(profile)
    #setup_all_feeds()
    generate_config(profile)
    generate_files(profile)

    print("Running make defconfig")
    if run(["make", "defconfig"]).returncode:
        die(f"Error running make defconfig")

    if call("grep 'CONFIG_TARGET_PROFILE' .config|grep %s" % (profile['profile']), shell=True) != 0:
        if call("grep 'CONFIG_TARGET_%s_%s_%s' .config" % (profile['target'], profile['subtarget'], profile['profile']), shell=True) != 0:
            die(f"Error: Cant not find the profile '%s'! Please check again!" % (profile['profile']))

    for package in profile['packages']:
        if call("grep 'CONFIG_PACKAGE_%s=y' .config" % (package), shell=True) != 0:
            die(f"Error: the package '%s' is not found or is not configured!" % (package))
