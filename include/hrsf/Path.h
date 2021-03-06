#pragma once
#include <array>
#include <vector>
#include <glm/vec3.hpp>

namespace hrsf
{
	struct PathSection
	{
		// time in seconds to go from the previous sections position to this sections positin;
		float time;
		glm::vec3 position;
	};

	/// describes the path an object takes.
	/// 
	/// For position paths (getPosition()):
	/// example:
	/// { "sections": [
	///	{"time": 4, "pos" : [1.0, 0.0, 0.0] }
	/// ]}
	/// The path (always) starts at [0, 0, 0].
	/// After 4 seconds, the path will arrive at [1.0, 0.0, 0.0]
	/// It will then reset back to [0, 0, 0] (if there is now next section)
	/// If the underlying component has a position and a positionPath attribute,
	/// the actual position should be: position + positionPath.getPosition();
	///
	/// For lookAt paths (getLookAt()):
	/// If the path contains only one element, the lookAt
	/// will be fixed to that position.
	/// Otherwise, the starting look at will be the "pos" of the last section.
	/// After "time" of the first section, the lookAt will be "pos" of the first section
	/// The lookAt will always go around in a circle between all sections.
	/// The lookAt should be relative to the origin ([0.0, 0.0, 0.0]).
	/// The actual lookAt should be computed by: position + lookAt.getLookAt(); and direction should be ignored
	class Path
	{
	public:
		Path(std::vector<PathSection> section, float scale)
			:
		m_sections(std::move(section)),
		m_scale(scale)
		{
			m_isCircle = m_sections.empty()?false:m_sections.back().position == glm::vec3(0.0f);
		}
		Path() = default;
		
		void update(float dt)
		{
			if (m_sections.empty()) return;

			m_time += dt;
			// find the section we are in at the moment
			while(m_sections[m_curSection].time < m_time)
			{
				m_time -= m_sections[m_curSection].time;
				m_curSection = (m_curSection + 1) % m_sections.size();
			}
		}
		glm::vec3 getPosition() const
		{
			if (m_sections.empty()) return glm::vec3(0.0f);
			if(m_sections.size() == 1)
			{
				// linear interpolation
				return m_sections.front().position * (m_time / m_sections.front().time);
			}
			// spline interpolation
			// previous point
			const auto preLeft = getPoint(int(m_curSection) - 1);
			const auto left = getPoint(int(m_curSection));
			const auto right = getPoint(int(m_curSection) + 1);
			const auto postRight = getPoint(int(m_curSection) + 2);

			// control point 1
			const auto cp1 = left + (right - preLeft) / 6.0f;
			const auto cp2 = right + (left - postRight) / 6.0f;

			// get spline
			return getBezierPoint(left, cp1, cp2, right, m_time / m_sections[m_curSection].time);
		}
		glm::vec3 getLookAt() const
		{
			if (m_sections.empty()) return glm::vec3(0.0f);
			if(m_sections.size() == 1)
			{
				// only look at one position
				return m_sections.front().position;
			}

			// previous point
			const auto preLeft = getLookAtPoint(int(m_curSection) - 2);
			const auto left = getLookAtPoint(int(m_curSection) - 1);
			const auto right = getLookAtPoint(int(m_curSection));
			const auto postRight = getLookAtPoint(int(m_curSection) + 1);

			// control point 1
			const auto cp1 = left + (right - preLeft) / 6.0f;
			const auto cp2 = right + (left - postRight) / 6.0f;

			// get spline
			return getBezierPoint(left, cp1, cp2, right, m_time / m_sections[m_curSection].time);
		}
		const std::vector<PathSection>& getSections() const
		{
			return m_sections;
		}
		float getScale() const
		{
			return m_scale;
		}
		bool isStatic() const
		{
			return m_sections.empty();
		}
		// throws an exception if negative section times are present
		void verify() const
		{
			for (auto& s : m_sections)
			{
				if (s.time <= 0.0f)
					throw std::runtime_error("path section times must be greater than zero");
			}
		}
	private:
		// returns path anchor point (there is one more point the section because paths always start at vec3(0))
		glm::vec3 getPoint(int index) const
		{
			if(m_isCircle)
			{
				index -= 1;
				while (index < 0) index += int(m_sections.size());

				// repeat points (its a circle, so 0 is already included)
				size_t i = size_t(index) % (m_sections.size());
				return m_sections[i].position * m_scale;
			}
			else
			{
				// clamp start and end points
				if (index <= 0) return glm::vec3(0.0f);
				return m_sections[std::min(size_t(index), m_sections.size() - 1)].position * m_scale;
			}
		}
		glm::vec3 getLookAtPoint(int index) const
		{
			while (index < 0) index += int(m_sections.size());
			// look at points are always in circle
			size_t i = size_t(index) % (m_sections.size());
			return m_sections[i].position * m_scale;
		}

		static glm::vec3 getBezierPoint(const glm::vec3& left, const glm::vec3& cp1, const glm::vec3& cp2, const glm::vec3& right, float t)
		{
			assert(t >= 0.0f);
			assert(t <= 1.0f);
			const auto t2 = t * t;
			const auto t3 = t2 * t;
			const auto tinv = 1.0f - t;
			const auto tinv2 = tinv * tinv;
			const auto tinv3 = tinv2 * tinv;

			return tinv3 * left + 3.0f * t * tinv2 * cp1 + 3.0f * t2 * tinv * cp2 + t3 * right;
		}
	private:
		std::vector<PathSection> m_sections;
		size_t m_curSection = 0;
		float m_scale;
		float m_time = 0.0f;
		bool m_isCircle;
	};
}
