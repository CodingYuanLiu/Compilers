## Diary
* 12.5 morning
Add emit,codegen. Add procEntryexit1 to do view_shift. Do not finish fp register and procEntryExit2,3.
> Caution: Jzy did not do procEntryexit1,2,3 but vinx did. Before a function need to: viewshift(PEE1),savecallee saved register(codegen),adjust stack register(PEE3),record sink variable(PEE2).
> PEE2 may cause segmentation fault.