#include "RenderThread.hpp"
#include "logger.hpp"

static std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4 proj, const glm::mat4 view)
{
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustum_corners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt =
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                frustum_corners.push_back(pt / pt.w);
            }
        }
    }

    return frustum_corners;
}

std::vector<glm::mat4> RenderThread::getCSMLightViewProjMatrices(
	const glm::vec3 & light_dir,
	const std::vector<float> & split,
	const glm::mat4 & camera_view,
	const float cam_fov,
	const float cam_ratio,
	const float cam_near_plane,
	const float cam_far_plane
)
{
	const float near_far_diff = cam_far_plane - cam_near_plane;

	std::vector<glm::mat4> light_view_proj_matrices;
	for (size_t i = 0; i + 1 < split.size(); i++)
	{
		float sub_frustum_near_plane = cam_near_plane + near_far_diff * split[i];
		float sub_frustum_far_plane = cam_near_plane + near_far_diff * split[i + 1];

		std::vector<glm::vec4> frustum_corners = getFrustumCornersWorldSpace(
			glm::perspective(cam_fov, cam_ratio, sub_frustum_near_plane, sub_frustum_far_plane),
			camera_view
		);

		glm::vec3 sub_frustum_center = glm::vec3(0.0f);
		for (const auto & corner : frustum_corners)
		{
			sub_frustum_center += glm::vec3(corner);
		}
		sub_frustum_center /= static_cast<float>(frustum_corners.size());

		// LOG_DEBUG("sub_frustum_center: " << sub_frustum_center.x << " " << sub_frustum_center.y << " " << sub_frustum_center.z);


		const glm::mat4 light_view = glm::lookAt(
			sub_frustum_center - light_dir,
			sub_frustum_center,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();
		for (const glm::vec4 & v : frustum_corners)
		{
			const glm::vec4 trf = light_view * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		// Tune this parameter according to the scene
		constexpr float zMult = 100.0f;
		if (minZ < 0)
		{
			minZ *= zMult;
		}
		else
		{
			minZ /= zMult;
		}
		if (maxZ < 0)
		{
			maxZ /= zMult;
		}
		else
		{
			maxZ *= zMult;
		}

		// LOG_DEBUG(i << " minX: " << minX << " maxX: " << maxX << " minY: " << minY << " maxY: " << maxY << " minZ: " << minZ << " maxZ: " << maxZ);

		const glm::mat4 light_proj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

		light_view_proj_matrices.push_back(light_proj * light_view);
	}

	return light_view_proj_matrices;
}
