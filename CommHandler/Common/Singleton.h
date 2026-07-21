#ifndef _SINGLETON_H_
#define _SINGLETON_H_


template <typename T>
class Singleton
{
public:
	static T* getInstance()
	{
		static T s_instance;
		return &s_instance;
	}

	Singleton(T&&) = delete;
	Singleton(const T&) = delete;
	void operator= (const T&) = delete;

protected:
	Singleton() = default;
	virtual ~Singleton() = default;
};
#endif