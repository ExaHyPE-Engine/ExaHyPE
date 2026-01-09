from sys import argv
from os.path import splitext

if len(argv) != 2 or argv[1] == "help":
    print("\tpositional argument: filepath to MSISE data")
    quit(0)
try:
    with open(argv[1], "r") as f:
        replacedLines = []
        for line in f:
            replacedLine = ""
            lastAlphaNum = True
            for c in line:
                if c == " " and lastAlphaNum:
                    replacedLine += ";"
                    lastAlphaNum = False
                elif c == " " and not lastAlphaNum:
                    pass
                else:
                    replacedLine += c
                    lastAlphaNum = True
            replacedLines.append(replacedLine)

        CSVFilename = splitext(argv[1])[0] + ".csv"
        try:
            with open(CSVFilename, "w") as csv:
                for line in replacedLines:
                    csv.write(line)
        except OSError:
            print(f"Could not create CSV file at {CSVFilename}.")

    print(
        "NOTE: The separator ';' might not be inserted in the head line between 'Heden(cm-3)' and 'Arden(cm-3)'. This does not influence the csv file for usage in ExaHyPE 2."
    )

except OSError:
    print(
        f'Could not open file at {argv[1]}. For help, use first positional argument "help".'
    )
