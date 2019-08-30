# papa-shango
Force Linux processes to enter the dark world of Papa Shango.

https://www.youtube.com/watch?v=hZSAzWvFpoE

## usage
You must write your payloads in assembler. NULL bytes do not matter for this.

```
make
cat papa-shango shellcode > payload
chmod +x payload
./payload <pid>
```

This will use ptrace() to inject shellcode into a running process.

