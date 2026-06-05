#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace TEN::ObjectParameters
{
	enum class ObjectParameterValueType
	{
		None,
		Boolean,
		Integer,
		Float,
		String
	};

	struct ObjectParameterValue
	{
		ObjectParameterValueType Type = ObjectParameterValueType::None;
		bool BooleanValue = false;
		int IntegerValue = 0;
		float FloatValue = 0.0f;
		std::string StringValue = {};

		static ObjectParameterValue FromBoolean(bool value)
		{
			auto result = ObjectParameterValue{};
			result.Type = ObjectParameterValueType::Boolean;
			result.BooleanValue = value;
			return result;
		}

		static ObjectParameterValue FromInteger(int value)
		{
			auto result = ObjectParameterValue{};
			result.Type = ObjectParameterValueType::Integer;
			result.IntegerValue = value;
			return result;
		}

		static ObjectParameterValue FromFloat(float value)
		{
			auto result = ObjectParameterValue{};
			result.Type = ObjectParameterValueType::Float;
			result.FloatValue = value;
			return result;
		}

		static ObjectParameterValue FromString(std::string value)
		{
			return ObjectParameterValue
			{
				ObjectParameterValueType::String,
				false,
				0,
				0.0f,
				std::move(value)
			};
		}
	};

	struct ObjectParameterObjectRef
	{
		int ItemIndex = -1;
		std::string ScriptName = {};
		std::string LuaReference = {};
		std::string ObjectId = {};

		bool IsValid() const
		{
			return ItemIndex >= 0 || !ScriptName.empty() || !LuaReference.empty() || !ObjectId.empty();
		}

		bool Matches(const ObjectParameterObjectRef& other) const
		{
			if (ItemIndex >= 0 && other.ItemIndex >= 0)
				return ItemIndex == other.ItemIndex;

			if (!ScriptName.empty() && !other.ScriptName.empty())
				return ScriptName == other.ScriptName;

			if (!LuaReference.empty() && !other.LuaReference.empty())
				return LuaReference == other.LuaReference;

			if (!ObjectId.empty() && !other.ObjectId.empty())
				return ObjectId == other.ObjectId;

			return false;
		}
	};

	struct ObjectParameterEntry
	{
		std::string ProviderId = {};
		std::string DefinitionSetId = {};
		ObjectParameterObjectRef Object = {};
		std::string ParameterId = {};
		ObjectParameterValue Value = {};

		bool IsValid() const
		{
			return !ProviderId.empty() && !ParameterId.empty() && Object.IsValid();
		}
	};

	using ObjectParameterConsumerCallback = std::function<void(const ObjectParameterEntry&)>;

	struct ObjectParameterConsumer
	{
		std::string ProviderId = {};
		std::string ConsumerId = {};
		ObjectParameterConsumerCallback Callback = {};
	};

	class ObjectParameterRegistry
	{
	public:
		void Clear()
		{
			_entries.clear();
			_consumers.clear();
		}

		void ClearEntries()
		{
			_entries.clear();
		}

		void ClearProviderEntries(const std::string& providerId)
		{
			_entries.erase(
				std::remove_if(_entries.begin(), _entries.end(), [&providerId](const auto& entry)
				{
					return entry.ProviderId == providerId;
				}),
				_entries.end());
		}

		bool AddEntry(ObjectParameterEntry entry)
		{
			if (!entry.IsValid())
				return false;

			_entries.push_back(std::move(entry));
			return true;
		}

		void AddEntries(const std::vector<ObjectParameterEntry>& entries)
		{
			for (const auto& entry : entries)
				AddEntry(entry);
		}

		const std::vector<ObjectParameterEntry>& GetEntries() const
		{
			return _entries;
		}

		std::vector<ObjectParameterEntry> GetEntriesByProvider(const std::string& providerId) const
		{
			auto entries = std::vector<ObjectParameterEntry>{};

			for (const auto& entry : _entries)
			{
				if (entry.ProviderId == providerId)
					entries.push_back(entry);
			}

			return entries;
		}

		std::vector<ObjectParameterEntry> GetEntriesByObject(const ObjectParameterObjectRef& objectRef) const
		{
			auto entries = std::vector<ObjectParameterEntry>{};

			for (const auto& entry : _entries)
			{
				if (entry.Object.Matches(objectRef))
					entries.push_back(entry);
			}

			return entries;
		}

		bool TryGetEntry(const std::string& providerId, const ObjectParameterObjectRef& objectRef, const std::string& parameterId, ObjectParameterEntry& result) const
		{
			for (const auto& entry : _entries)
			{
				if (entry.ProviderId == providerId && entry.ParameterId == parameterId && entry.Object.Matches(objectRef))
				{
					result = entry;
					return true;
				}
			}

			return false;
		}

		bool RegisterConsumer(std::string providerId, std::string consumerId, ObjectParameterConsumerCallback callback)
		{
			if (providerId.empty() || consumerId.empty() || !callback)
				return false;

			UnregisterConsumer(consumerId);

			_consumers.push_back(ObjectParameterConsumer
			{
				std::move(providerId),
				std::move(consumerId),
				std::move(callback)
			});

			return true;
		}

		bool UnregisterConsumer(const std::string& consumerId)
		{
			const auto oldSize = _consumers.size();

			_consumers.erase(
				std::remove_if(_consumers.begin(), _consumers.end(), [&consumerId](const auto& consumer)
				{
					return consumer.ConsumerId == consumerId;
				}),
				_consumers.end());

			return _consumers.size() != oldSize;
		}

		void DispatchEntry(const ObjectParameterEntry& entry) const
		{
			for (const auto& consumer : _consumers)
			{
				if (consumer.ProviderId == entry.ProviderId && consumer.Callback)
					consumer.Callback(entry);
			}
		}

		void DispatchProvider(const std::string& providerId) const
		{
			for (const auto& entry : _entries)
			{
				if (entry.ProviderId == providerId)
					DispatchEntry(entry);
			}
		}

	private:
		std::vector<ObjectParameterEntry> _entries = {};
		std::vector<ObjectParameterConsumer> _consumers = {};
	};

	inline ObjectParameterRegistry& GetObjectParameterRegistry()
	{
		static auto registry = ObjectParameterRegistry{};
		return registry;
	}
}
