Dim fso
Set fso = CreateObject("Scripting.FileSystemObject")

Set File = fso.CreateTextFile("\epoc32\winscw\c\tswi\ﾅｿﾄ.txt", true)
File.WriteLine("osver")
File.Close()

