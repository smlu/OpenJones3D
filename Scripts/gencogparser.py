# Script generates COG script parser files: sithCogFlex.c, sithCogYacc.h and sithCogYacc.h

#!/usr/bin/env python3
import os, sys
import subprocess
import shutil
import argparse
import textwrap

# Default constants (can be overridden via command line arguments)
SCRUPT_ROOT_DIR = os.path.dirname(os.path.abspath(__file__))

DEFAULT_ROOT_DIR  = os.path.abspath(os.path.join(SCRUPT_ROOT_DIR, "../Libs/sith/Cog"))
DEFAULT_FLEX_PATH = os.path.abspath(os.path.join(SCRUPT_ROOT_DIR, "../build/Programs/flex/flex"))
DEFAULT_YACC_PATH = os.path.abspath(os.path.join(SCRUPT_ROOT_DIR, "../build/Programs/byacc/yacc"))

def parse_arguments():
    parser = argparse.ArgumentParser(description="Process Flex and Yacc files for Cog")
    parser.add_argument("--root", default=DEFAULT_ROOT_DIR, help="Root folder containing cog.l, cog.y, and flex.skel files")
    parser.add_argument("--flex", default=DEFAULT_FLEX_PATH, help="Path to flex executable")
    parser.add_argument("--yacc", default=DEFAULT_YACC_PATH, help="Path to yacc executable")
    return parser.parse_args()

def main() -> bool:
    args = parse_arguments()
    
    ROOT_FOLDER = args.root
    FLEX_PATH = args.flex
    YACC_PATH = args.yacc
    
    print(f"Starting Flex and Yacc processing in {ROOT_FOLDER}...")
    
    # Ensure the input files exist
    cog_l_path = os.path.join(ROOT_FOLDER, "cog.l")
    cog_y_path = os.path.join(ROOT_FOLDER, "cog.y")
    flex_skel_path = os.path.join(ROOT_FOLDER, "flex.skel")
    sithcogyacc_h_path = os.path.join(ROOT_FOLDER, "sithCogYacc.h")
    sithcogyacc_c_path = os.path.join(ROOT_FOLDER, "sithCogYacc.c")
    sithcogflex_c_path = os.path.join(ROOT_FOLDER, "sithCogFlex.c")
    
    if not os.path.exists(cog_l_path):
        print(f"Error: cog.l file not found at {cog_l_path}")
        return False
        
    if not os.path.exists(cog_y_path):
        print(f"Error: cog.y file not found at {cog_y_path}")
        return False
        
    if not os.path.exists(flex_skel_path):
        print(f"Error: flex.skel file not found at {flex_skel_path}")
        return False
    
    # Run flex on cog.l with custom skeleton
    print("Running flex on cog.l ...")
    flex_result = subprocess.run(
        [FLEX_PATH, f"-S{flex_skel_path}", "-Li", cog_l_path], #-L no linenumbers, -i case insensitive
        capture_output=True, 
        text=True
    )
    
    print("Defining sithCogYacc.h ...")
    if flex_result.returncode != 0:
        print(f"Error running flex: {flex_result.stderr}")
        return False

    # Run yacc/bison on cog.y
    print("Running yacc on cog.y...")
    yacc_result = subprocess.run([YACC_PATH, "-dl", cog_y_path], capture_output=True, text=True)
    
    if yacc_result.returncode != 0:
        print(f"Error running yacc: {yacc_result.stderr}")
        return False
    

    
    # Rename the output files
    if os.path.exists("lex.yy.c"):
        shutil.move("lex.yy.c", sithcogflex_c_path)
    else:
        print("Error: lex.yy.c not found")
        return False
        
    if os.path.exists("y.tab.c"):
        shutil.move("y.tab.c", sithcogyacc_c_path)
    else:
        print("Error: y.tab.c not found")
        return False
        
    if os.path.exists("y.tab.h"):
        shutil.move("y.tab.h", sithcogyacc_h_path)
    else:
        print("Error: y.tab.h not found")
        return False
    
    # Modifying sithCogYacc.h

    with open(sithcogyacc_h_path, "r") as f:
            ytab_content = f.read()
    
    with open(sithcogyacc_h_path, "w") as f:
        f.write(textwrap.dedent("""
            #ifndef SITH_COG_YACC_H
            #define SITH_COG_YACC_H

            #include <rdroid/types.h>
            #include <sith/types.h>
            #include <std/types.h>

            J3D_EXTERN_C_START

            {ytab_content}
            extern size_t yylinenum;
            extern int yyleng;
            extern tFileHandle yyin, yyout;

            int yyparse(void);
            int yylex(void);
            void yyrestart(tFileHandle input_file);
                                
            static void sithCogYacc_ResetGlobals(void){{}}

            J3D_EXTERN_C_END

            #endif //SITH_COG_YACC_H

        """).strip().format(ytab_content=ytab_content))

    print("Success!")
    return True

if __name__ == "__main__":
    ret = main()
    sys.exit(0 if ret else 1)