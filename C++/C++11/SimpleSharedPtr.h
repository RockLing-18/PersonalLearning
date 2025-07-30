#pragma once
#include <atomic>

/*
* 一个基于C++11标准的简化版shared_ptr实现
*/



template <typename T>
class SimpleSharedPtr {
private:
	T* m_ptr;                          // 指向实际对象的指针
	std::atomic<size_t>* m_refCount;  // 原子引用计数器
	void (*m_deleter)(T*);             // 自定义删除器

	// 释放资源：减少引用计数，若为0则销毁对象和控制块
	void release() {
		if (!m_refCount) return;
		if (--(*m_refCount) == 0) {
			if (m_deleter) m_deleter(m_ptr);  // 使用自定义删除器
			else delete m_ptr;            // 默认删除
			delete m_refCount;           // 销毁引用计数器
		}
		m_ptr = nullptr;
		m_refCount = nullptr;
	}

public:
	// 默认构造函数
	SimpleSharedPtr() : m_ptr(nullptr), m_refCount(nullptr), m_deleter(nullptr) {}

	// 构造函数（支持自定义删除器）
	explicit SimpleSharedPtr(T* raw_ptr, void (*del)(T*) = nullptr)
		: m_ptr(raw_ptr), m_refCount(new std::atomic<size_t>(1)), m_deleter(del) {}

	// 拷贝构造函数
	SimpleSharedPtr(const SimpleSharedPtr& other)
		: m_ptr(other.m_ptr), m_refCount(other.m_refCount), m_deleter(other.m_deleter) {
		if (m_refCount) (*m_refCount)++;
	}

	// 移动构造函数
	SimpleSharedPtr(SimpleSharedPtr&& other) noexcept
		: m_ptr(other.m_ptr), m_refCount(other.m_refCount), m_deleter(other.m_deleter) {
		other.m_ptr = nullptr;
		other.m_refCount = nullptr;
	}

	// 拷贝赋值运算符
	SimpleSharedPtr& operator=(const SimpleSharedPtr& other) {
		if (this != &other) {
			release();  // 释放当前资源
			m_ptr = other.m_ptr;
			m_refCount = other.m_refCount;
			m_deleter = other.m_deleter;
			if (m_refCount) (*m_refCount)++;
		}
		return *this;
	}

	// 移动赋值运算符
	SimpleSharedPtr& operator=(SimpleSharedPtr&& other) noexcept {
		if (this != &other) {
			release();
			m_ptr = other.m_ptr;
			m_refCount = other.m_refCount;
			m_deleter = other.m_deleter;
			other.m_ptr = nullptr;
			other.m_refCount = nullptr;
		}
		return *this;
	}

	// 析构函数
	~SimpleSharedPtr() { release(); }

	// 访问对象成员
	T* operator->() const { return m_ptr; }
	T& operator*() const { return *m_ptr; }

	// 获取原始指针（不影响引用计数）
	T* get() const { return m_ptr; }

	// 获取引用计数值
	size_t use_count() const {
		return m_refCount ? m_refCount->load() : 0;
	}

	// 重置指针（可指向新对象）
	void reset(T* new_ptr = nullptr, void (*del)(T*) = nullptr) {
		release();
		m_ptr = new_ptr;
		m_deleter = del;
		m_refCount = new_ptr ? new std::atomic<size_t>(1) : nullptr;
	}
};


/*test

{
	SimpleSharedPtr<int> ptr1;  // 创建空智能指针
	std::cout << "ptr1 use_count: " << ptr1.use_count() << std::endl; // 输出 0
}

{
	SimpleSharedPtr<int> ptr2(new int(42));  // 引用计数初始化为 1
	std::cout << "ptr2 value: " << *ptr2 << ", use_count: " << ptr2.use_count() << std::endl;
	// 输出：42, use_count: 1

	SimpleSharedPtr<int> ptr3 = ptr2;  // 引用计数 +1
	std::cout << "ptr2 use_count: " << ptr2.use_count() << std::endl; // 输出 2
	std::cout << "ptr3 use_count: " << ptr3.use_count() << std::endl; // 输出 2

	SimpleSharedPtr<std::string> ptr4(new std::string("Hello"));
	std::cout << *ptr4 << std::endl;        // 解引用输出：Hello
	std::cout << ptr4->size() << std::endl; // 成员访问输出：5
}

{
	管理文件句柄（自定义删除器）
	void file_deleter(FILE* fp) { 
	    if (fp) fclose(fp); 
	}

	SimpleSharedPtr<FILE> file_ptr(fopen("data.txt", "r"), file_deleter);
}

*/

