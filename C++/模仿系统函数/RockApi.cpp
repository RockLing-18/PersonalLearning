#include <assert.h>
char* RockStrcpy(char* dest, char const* source)
{
	assert(dest && source);

	char* ret = dest;  // 记录初始地址
	while ((*dest++ = *source++) != '\0');  // 逐字符拷贝
	return ret;  // 返回初始地址[1,6](@ref)
}


char* RockStrncpy(char* dest, char const* source, size_t size)
{
	assert(dest && source);

	char* ret = dest;  // 记录初始地址
	while (size-- && (*dest++ = *source++)); // 逐字符拷贝
	
	*dest = '\0';
	return ret;  // 返回初始地址[1,6](@ref)
}

void* RockMemcpy(void* dest, void const* source, size_t size)
{
	assert(dest && source);

	void* ret = dest;  // 记录初始地址
	char* d = (char*)dest;     // 按字节操作
	const char* s = (const char*)source;
	// 处理内存重叠：若dest在src之后且重叠，从后向前复制
	if (d > s && d < s + size)
	{
		d += size - 1;
		s += size - 1;
		while (size--)
		{
			*d-- = *s--; // 反向复制
		}
	}
	else
	{
		while (size--)
		{
			*d++ = *s++; // 正向复制
		}
	}

	return ret;  // 返回初始地址[1,6](@ref)
}

void* RockMemmove(void* dest, const void* source, size_t count) {
	char* d = (char*)dest;
	const char* s = (const char*)source;
	if (s == d || count == 0) return dest;  // 无操作条件

	if (d > s && d < s + count) 
	{
		// 正向重叠：源在目标左侧
		d += count - 1;
		s += count - 1;
		while (count--)
		{
			*d-- = *s--; // 反向复制
		}
	}
	else
	{
		// 无重叠或逆向重叠
		while (count--)
		{
			*d++ = *s++; // 正向复制
		}
	}

	return dest;
}
