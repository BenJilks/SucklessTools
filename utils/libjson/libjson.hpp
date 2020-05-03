#pragma once
#include <memory>
#include <map>
#include <vector>
#include <optional>

namespace Json
{

	enum class ValueType
	{
		Object,
		Array,
		String,
		Number,
		Boolean,
		Null
	};

	class Value
	{
	public:
		Value() {}

		template <typename T>
		bool is() const { return this_type() == T::type(); }

		template <typename T>
		inline T *as() 
		{
			if (!is<T>())
				return nullptr;

			return static_cast<T*>(this);
		}

		template <typename T>
		inline const T *as() const
		{
			if (!is<T>())
				return nullptr;

			return static_cast<const T*>(this);
		}	

		std::string to_string(bool pretty_print = false) const;
		float to_float() const;
		double to_double() const;
		int to_int() const;
		bool to_bool() const;

	protected:
		virtual ValueType this_type() const = 0;

	};

	class Null : public Value
	{
	public:
		Null()
			: Value()
		{
		}

		inline static ValueType type() { return ValueType::Null; }

	private:
		virtual ValueType this_type() const override { return type(); }	

	};

	class Object : public Value
	{
	public:
		Object()
			: Value()
		{
		}

		inline Value &get(const std::string &name) { return *data[name]; }
		inline void set(const std::string &name, std::shared_ptr<Value> &value) { data[name] = value; }

		template <typename T>
		inline std::shared_ptr<T> add(const std::string &name) 
		{ 
			auto value = std::make_shared<T>();
			data[name] = value;
			return value; 
		}

		void add(const std::string &name, const std::string str);
		void add(const std::string &name, const char *str);
		void add(const std::string &name, double number);
		void add(const std::string &name, bool boolean);
		void remove(const std::string &name) { data.erase(name); }
		bool contains(const std::string &name) { return data.find(name) != data.end(); }

		template <typename T>
		inline T *get_as(const std::string &name) 
		{ 
			auto value = data[name];
			if (!value)
				return nullptr;
			return value->as<T>();
		}

		auto begin() { return data.begin(); }
		const auto begin() const { return data.begin(); }
		auto end() { return data.end(); }
		const auto end() const { return data.end(); }

		inline static ValueType type() { return ValueType::Object; }

	private:
		virtual ValueType this_type() const override { return type(); }

		std::map<std::string, std::shared_ptr<Value>> data;

	};

	class Array : public Value
	{
	public:
		Array()
			: Value()
		{
		}

		inline Value &get(int index) { return *data[index]; }
		inline void set(int index, std::shared_ptr<Value> &value) 
		{
			if (data.size() <= index)
				data.resize(index);
			data[index] = value; 
		}

		inline void append(std::shared_ptr<Value> &value)
		{
			data.push_back(value);
		}

		template <typename T>
		inline std::weak_ptr<T> append() 
		{ 
			auto value = std::make_shared<T>();
			data.push_back(value);
			return value;
		}

		template <typename T>
		inline T *get_as(int index) { return data[index]->as<T>(); }

		auto begin() { return data.begin(); }
		const auto begin() const { return data.begin(); }
		auto end() { return data.end(); }
		const auto end() const { return data.end(); }

		inline static ValueType type() { return ValueType::Array; }

	private:
		virtual ValueType this_type() const override { return type(); }

		std::vector<std::shared_ptr<Json::Value>> data;

	};

	class String : public Value
	{
	public:
		String(const std::string &str) 
			: Value()
			, data(str)
		{
		}

		inline static ValueType type() { return ValueType::String; }

		inline std::string &get() { return data; }
		inline const std::string &get() const { return data; }
		inline void set(const std::string &str) { data = str; }
	
	private:
		virtual ValueType this_type() const override { return type(); }
		
		std::string data;

	};

	class Number : public Value
	{
	public:
		Number(double number) 
			: Value()
			, data(number)
		{
		}

		inline static ValueType type() { return ValueType::Number; }

		inline double &get() { return data; }
		inline const double &get() const { return data; }
		inline void set(double d) { data = d; }
	
	private:
		virtual ValueType this_type() const override { return type(); }
		
		double data;

	};

	class Boolean : public Value
	{
	public:
		Boolean(bool boolean) 
			: Value()
			, data(boolean)
		{
		}

		inline static ValueType type() { return ValueType::Boolean; }

		inline bool &get() { return data; }
		inline const bool &get() const { return data; }
		inline void set(bool b) { data = b; }
	
	private:
		virtual ValueType this_type() const override { return type(); }
		
		bool data;

	};

	std::shared_ptr<Value> parse(const std::string &file_path);
	std::ostream &operator << (std::ostream &out, const Value &value);
	std::ostream &operator << (std::ostream &out, std::shared_ptr<Value> &value);

}

