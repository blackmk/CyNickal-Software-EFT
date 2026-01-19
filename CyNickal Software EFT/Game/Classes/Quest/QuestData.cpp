#include "pch.h"
#include "QuestData.h"
#include <fstream>
#include <iostream>

namespace Quest
{
	bool QuestDatabase::LoadFromFile(const std::string& filePath)
	{
		try
		{
			std::ifstream file(filePath);
			if (!file.is_open())
			{
				std::cout << "[QuestDatabase] Failed to open file: " << filePath << std::endl;
				return false;
			}

			nlohmann::json json;
			file >> json;
			file.close();

			if (json.contains("tasks") && json["tasks"].is_array())
			{
				ParseTasks(json["tasks"]);
				BuildZoneIndex();
				BuildQuestItemIndex();
				m_bLoaded = true;
				std::cout << "[QuestDatabase] Loaded " << m_Tasks.size() << " tasks from file" << std::endl;
				return true;
			}

			std::cout << "[QuestDatabase] Invalid JSON format - missing 'tasks' array" << std::endl;
			return false;
		}
		catch (const std::exception& e)
		{
			std::cout << "[QuestDatabase] Error loading file: " << e.what() << std::endl;
			return false;
		}
	}

	bool QuestDatabase::LoadFromJson(const std::string& jsonStr)
	{
		try
		{
			nlohmann::json json = nlohmann::json::parse(jsonStr);

			if (json.contains("tasks") && json["tasks"].is_array())
			{
				ParseTasks(json["tasks"]);
				BuildZoneIndex();
				BuildQuestItemIndex();
				m_bLoaded = true;
				std::cout << "[QuestDatabase] Loaded " << m_Tasks.size() << " tasks from JSON" << std::endl;
				return true;
			}

			std::cout << "[QuestDatabase] Invalid JSON format - missing 'tasks' array" << std::endl;
			return false;
		}
		catch (const std::exception& e)
		{
			std::cout << "[QuestDatabase] Error parsing JSON: " << e.what() << std::endl;
			return false;
		}
	}

	void QuestDatabase::ParseTasks(const nlohmann::json& tasksJson)
	{
		m_Tasks.clear();

		for (const auto& taskJson : tasksJson)
		{
			try
			{
				TaskDefinition task;
				task.Id = taskJson.value("id", "");
				task.Name = taskJson.value("name", "");

				if (task.Id.empty())
					continue;

				// Parse objectives
				if (taskJson.contains("objectives") && taskJson["objectives"].is_array())
				{
					for (const auto& objJson : taskJson["objectives"])
					{
						TaskObjective obj;
						obj.Id = objJson.value("id", "");
						obj.Type = objJson.value("type", "");
						obj.Description = objJson.value("description", "");
						obj.Count = objJson.value("count", 0);
						obj.bFoundInRaid = objJson.value("foundInRaid", false);

						// Parse zones
						if (objJson.contains("zones") && objJson["zones"].is_array())
						{
							for (const auto& zoneJson : objJson["zones"])
							{
								TaskZone zone;
								zone.Id = zoneJson.value("id", "");

								if (zoneJson.contains("map") && zoneJson["map"].is_object())
								{
									zone.MapNameId = zoneJson["map"].value("nameId", "");
								}

								if (zoneJson.contains("position") && zoneJson["position"].is_object())
								{
									const auto& posJson = zoneJson["position"];
									zone.Position.x = posJson.value("x", 0.0f);
									zone.Position.y = posJson.value("y", 0.0f);
									zone.Position.z = posJson.value("z", 0.0f);
								}

								if (!zone.Id.empty() && !zone.MapNameId.empty())
								{
									obj.Zones.push_back(zone);
								}
							}
						}

						// Parse quest item (findQuestItem)
						if (objJson.contains("questItem") && objJson["questItem"].is_object())
						{
							const auto& itemJson = objJson["questItem"];
							obj.QuestItemRef.Id = itemJson.value("id", "");
							obj.QuestItemRef.Name = itemJson.value("name", "");
							obj.QuestItemRef.ShortName = itemJson.value("shortName", "");
						}

						// Parse regular item (findItem)
						if (objJson.contains("item") && objJson["item"].is_object())
						{
							const auto& itemJson = objJson["item"];
							obj.Item.Id = itemJson.value("id", "");
							obj.Item.Name = itemJson.value("name", "");
							obj.Item.ShortName = itemJson.value("shortName", "");
						}

						// Parse marker item (mark)
						if (objJson.contains("markerItem") && objJson["markerItem"].is_object())
						{
							const auto& itemJson = objJson["markerItem"];
							obj.MarkerItem.Id = itemJson.value("id", "");
							obj.MarkerItem.Name = itemJson.value("name", "");
							obj.MarkerItem.ShortName = itemJson.value("shortName", "");
						}

						task.Objectives.push_back(obj);
					}
				}

				m_Tasks[task.Id] = task;
			}
			catch (const std::exception& e)
			{
				std::cout << "[QuestDatabase] Error parsing task: " << e.what() << std::endl;
			}
		}
	}

	void QuestDatabase::BuildZoneIndex()
	{
		m_TaskZones.clear();

		for (const auto& [taskId, task] : m_Tasks)
		{
			for (const auto& obj : task.Objectives)
			{
				for (const auto& zone : obj.Zones)
				{
					if (!zone.MapNameId.empty() && !zone.Id.empty())
					{
						m_TaskZones[zone.MapNameId][zone.Id] = zone.Position;
					}
				}
			}
		}
	}

	void QuestDatabase::BuildQuestItemIndex()
	{
		m_QuestItemIds.clear();

		for (const auto& [taskId, task] : m_Tasks)
		{
			for (const auto& obj : task.Objectives)
			{
				// Add quest items from findQuestItem objectives
				if (!obj.QuestItemRef.Id.empty())
				{
					m_QuestItemIds.insert(obj.QuestItemRef.Id);
				}
			}
		}

		std::cout << "[QuestDatabase] Indexed " << m_QuestItemIds.size() << " quest item IDs" << std::endl;
	}
}
