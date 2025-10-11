Import("env")

def dump_elf_sections(source, target, env):
    # Get the ELF file path (e.g., .pio/build/<env>/firmware.elf)
    elf_path = target[0].get_abspath() if isinstance(target, list) else target.get_abspath()
    
    # Use the toolchain's objdump (replace 'objcopy' with 'objdump' since they share the base path)
    objdump = env.subst("${OBJCOPY}").replace("objcopy", "objdump")
    
    # Command to list sections (-h) and save to file (or print to console)
    cmd = f"{objdump} -h {elf_path} > sections.txt"
    env.Execute(cmd)
    
    # Optional: Print to console instead
    # cmd = f"{objdump} -h {elf_path}"
    # env.Execute(cmd)
    
    print(f"ELF sections dumped to sections.txt for {elf_path}")

# Hook after ELF is built
env.AddPostAction("$PROGPATH", dump_elf_sections)