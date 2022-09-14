@echo "CLANG-FORMAT SCRIPT FOR ALL .H .CPP FILES"
@clang-format -i -style=Microsoft --sort-includes *.cpp *.h && @echo "Formatting completed!" || @echo "Formatting failed!"
@pause