Dim WshShell, oExec
Dim fso_result, fso_pkg 

Set fso_pkg = CreateObject("Scripting.FileSystemObject")
Set File = fso_pkg.CreateTextFile("ゎわこんァア龐龑.pkg", true)
File.WriteLine (";Languages")
File.WriteLine ("&JA")
File.WriteLine ("")
File.WriteLine (";Header")
File.WriteLine ("#{""names large sample UTF8""}, (0xA2000222), 1, 2, 3, TYPE=SA")
File.WriteLine ("")
File.WriteLine ("%{""Vendor""}")
File.WriteLine (":""Vendor""")
File.WriteLine ("")
File.WriteLine (";Mixed UTF8 characters")
File.WriteLine ("""\epoc32\winscw\c\tswi\utf8.txt""-""!:\utf8.txt""")
File.Close()

Set WshShell = CreateObject("WScript.Shell")
Set oExec = WshShell.Exec("makesis -v ゎわこんァア龐龑.pkg")

Do While oExec.Status = 0 
	WScript.Sleep 100 
Loop 

