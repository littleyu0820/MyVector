#include <iostream>
#include <memory>
#include <utility>
#include <string>

//設計一個小型的vector

//V2 優化

class strVec
{
public:
	strVec():
		first_element(nullptr), after_last_element(nullptr), vector_end(nullptr) {}
	strVec(const strVec&);
	strVec(strVec&&) noexcept; //使用noexcept的原因在於移動的過程中舊有資料(空間)可能就發生變化了，編譯器無法接受這種例外，認為有風險
							   //但我們告訴編譯器沒有問題，所以才使用noexcept
	strVec& operator=(const strVec&) &; //強制回傳左值
	strVec& operator=(strVec&&) noexcept;

	~strVec()
	{
		free();
	}

	void push_back(const std::string&); //拷貝
	void push_back(std::string&&); //移動
	size_t get_size() const
	{
		return after_last_element - first_element;
	}

	size_t capacity() const
	{
		return vector_end - first_element;
	}

	std::string* begin() const
	{
		return first_element;
	}

	std::string* end() const
	{
		return after_last_element;
	}
private:

	static std::allocator<std::string> alloc; //用來配置空記憶體
	void check_alloc_size() //檢查容量
	{
		if (get_size() == capacity()) //如果儲存元素數量已經等於容量大小
			reallocate(); //重新分配
	}
	std::pair<std::string*, std::string*> copy_alloc(const std::string*, const std::string*); //copy

	void free();
	void reallocate();

	std::string* first_element; //第一個元素
	std::string* after_last_element; //最後一個元素的下一個空位置
	std::string* vector_end; //最後一個空位置

};

std::allocator<std::string> strVec::alloc;

void strVec::push_back(const std::string& s)
{
	check_alloc_size(); //只有增加元素時才有可能大小不足
	alloc.construct(after_last_element++, s); //將s拷貝到first_free上然後移動first_free
}

void strVec::push_back(std::string&& s)
{
	check_alloc_size(); //只有增加元素時才有可能大小不足
	alloc.construct(after_last_element++, std::move(s)); //將s移動到first_free然後移動first_free
}

std::pair<std::string*, std::string*> strVec::copy_alloc(const std::string* b, const std::string* e) //used to copy
{
	auto data = alloc.allocate(e - b); //分配一塊空間給目標物件
	return { data, std::uninitialized_copy(b,e,data) }; //回傳目標物件的第一個元素跟尾後
														//uninitialized_copy(b,e,data)把b到e複製到data裡面
}

void strVec::free()
{
	if (first_element) // if there are elements in the vector means that begin is not null
	{
		for (auto p = after_last_element; p != first_element;) //delete the element from the last element
			alloc.destroy(--p);

		alloc.deallocate(first_element, vector_end - first_element); //deallocate from element to cap (begin to end)
	}
}


strVec::strVec(const strVec& v) //複製
{
	auto new_data = copy_alloc(v.begin(), v.end());
	first_element = new_data.first;
	after_last_element = vector_end = new_data.second;
}

strVec::strVec(strVec&& v) noexcept: //移動
	first_element(v.first_element), after_last_element(v.after_last_element), vector_end(v.vector_end)
{
	v.first_element = v.after_last_element = v.vector_end = nullptr;
}

strVec& strVec::operator=(const strVec& rhs) &
{
	auto data = copy_alloc(rhs.begin(), rhs.end());
	free();
	first_element = data.first;
	after_last_element = vector_end = data.second;

	return *this;
}


strVec& strVec::operator=(strVec&& rhs) noexcept 
{
	if (this != &rhs) //檢查this所指的位址與rhs是否相同
	{
		free(); //釋放左邊運算元的記憶體(this.free)
		first_element = rhs.first_element;
		after_last_element = rhs.after_last_element;
		vector_end = rhs.vector_end;
		rhs.first_element = rhs.after_last_element = rhs.vector_end = nullptr;
	}

	return *this;
}

void strVec::reallocate()

{
	auto new_capacity = (get_size() == 0) ? 1 : get_size() * 2; //確認是否為新的vector
	auto first = alloc.allocate(new_capacity); //分配新的空間 定義 回傳指標指向第一個元素(位址)
	auto last = std::uninitialized_copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), first); 
	//移動資料回傳尾後 alloc.construct會使用移動建構器
	
	free(); //釋放舊空間
	first_element = first; //更新指標指向新空間
	after_last_element = last;
	vector_end = first_element + new_capacity;

}

int main()
{

	strVec vec1, vec2, vec3;
	std::string s = "The test";
	//測試push_back
	vec1.push_back(s); //拷貝
	vec1.push_back("The test"); //移動
	vec2.push_back(s); //拷貝

	//測試assign
	std::cout << "Before assign Vec2..." << std::endl;
	for (auto val : vec2)
	{
		std::cout << val << std::endl;
	}

	vec2 = vec1; ///normal assign since vec1 is lvalue

	std::cout << "After assign..." << std::endl;
	std::cout << "Vec2:" << std::endl;
	for (auto val : vec2)
	{
		std::cout << val << std::endl;
	}

	//測試move
	std::cout << "Before move..." << std::endl;
	std::cout << "Vec1:" << std::endl;
	for (auto val : vec1)
	{
		std::cout << val << std::endl;
	}

	std::cout << "Vec3:" << std::endl;
	for (auto val : vec3)
	{
		std::cout << val << std::endl;
	}

	vec3 = std::move(vec1);
	std::cout << "After move..." << std::endl;
	std::cout << "Vec1:" << std::endl;
	if (vec1.get_size() == 0)
	{
		std::cout << "It's empty..." << std::endl;
	}
	else
	{
		for (auto val : vec1)
		{
			std::cout << val << std::endl;
		}
	}

	std::cout << "Vec3:" << std::endl;

	if (vec3.get_size() == 0)
	{
		std::cout << "It's empty..." << std::endl;
	}
	else
	{
		for (auto val : vec3)
		{
			std::cout << val << std::endl;
		}
	}


	return 0;
}
