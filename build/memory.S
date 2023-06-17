
memory.o：     文件格式 elf32-i386


Disassembly of section .text:

00000000 <__alloca>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	83 ec 18             	sub    $0x18,%esp
   6:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
   a:	75 19                	jne    25 <__alloca+0x25>
   c:	68 00 00 00 00       	push   $0x0
  11:	68 1c 00 00 00       	push   $0x1c
  16:	6a 06                	push   $0x6
  18:	68 0a 00 00 00       	push   $0xa
  1d:	e8 fc ff ff ff       	call   1e <__alloca+0x1e>
  22:	83 c4 10             	add    $0x10,%esp
  25:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
  2c:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
  33:	89 6d ec             	mov    %ebp,-0x14(%ebp)
  36:	89 65 e8             	mov    %esp,-0x18(%ebp)
  39:	8b 45 08             	mov    0x8(%ebp),%eax
  3c:	83 c0 03             	add    $0x3,%eax
  3f:	83 e0 fc             	and    $0xfffffffc,%eax
  42:	89 45 08             	mov    %eax,0x8(%ebp)
  45:	8b 45 ec             	mov    -0x14(%ebp),%eax
  48:	89 c2                	mov    %eax,%edx
  4a:	8b 45 08             	mov    0x8(%ebp),%eax
  4d:	01 d0                	add    %edx,%eax
  4f:	83 c0 0f             	add    $0xf,%eax
  52:	83 e0 f0             	and    $0xfffffff0,%eax
  55:	89 45 f4             	mov    %eax,-0xc(%ebp)
  58:	8b 45 f4             	mov    -0xc(%ebp),%eax
  5b:	8b 55 e8             	mov    -0x18(%ebp),%edx
  5e:	29 d0                	sub    %edx,%eax
  60:	83 c0 1c             	add    $0x1c,%eax
  63:	89 45 f0             	mov    %eax,-0x10(%ebp)
  66:	8b 45 f0             	mov    -0x10(%ebp),%eax
  69:	29 c4                	sub    %eax,%esp
  6b:	8b 55 08             	mov    0x8(%ebp),%edx
  6e:	89 54 24 08          	mov    %edx,0x8(%esp)
  72:	8b 55 04             	mov    0x4(%ebp),%edx
  75:	89 54 24 04          	mov    %edx,0x4(%esp)
  79:	8b 55 00             	mov    0x0(%ebp),%edx
  7c:	89 14 24             	mov    %edx,(%esp)
  7f:	8b 45 f4             	mov    -0xc(%ebp),%eax
  82:	5d                   	pop    %ebp
  83:	c3                   	ret    
  84:	8b 45 f4             	mov    -0xc(%ebp),%eax
  87:	c9                   	leave  
  88:	c3                   	ret    
