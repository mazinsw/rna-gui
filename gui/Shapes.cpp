#include "Shapes.h"
#include <rlgl.h>
#include <math.h>
#include <vector>

namespace shapes
{
	// largura da faixa suavizada, 1 pixel da tela em coordenadas da janela
	static float feather = 1;

	void beginFrame()
	{
		Vector2 scale = GetWindowScaleDPI();
		feather = scale.x > 0 ? 1 / scale.x : 1;
	}

	static Color transparent(Color color)
	{
		color.a = 0;
		return color;
	}

	static void vertex(Vector2 point, Color color)
	{
		rlColor4ub(color.r, color.g, color.b, color.a);
		rlVertex2f(point.x, point.y);
	}

	/**
	 * Triângulo em qualquer ordem, o raylib descarta os que não estão no sentido
	 * anti-horário da tela
	 */
	static void triangle(Vector2 a, Color ca, Vector2 b, Color cb, Vector2 c, Color cc)
	{
		float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
		if(cross == 0)
			return;
		vertex(a, ca);
		if(cross < 0)
		{
			vertex(b, cb);
			vertex(c, cc);
		}
		else
		{
			vertex(c, cc);
			vertex(b, cb);
		}
	}

	static void quad(Vector2 a, Color ca, Vector2 b, Color cb, Vector2 c, Color cc, Vector2 d,
		Color cd)
	{
		triangle(a, ca, b, cb, c, cc);
		triangle(a, ca, c, cc, d, cd);
	}

	static Vector2 add(Vector2 a, Vector2 b, float scale)
	{
		return Vector2{ a.x + b.x * scale, a.y + b.y * scale };
	}

	/**
	 * Normal de cada vértice de um contorno fechado, apontando para fora e
	 * alongada nos cantos para manter a largura das faixas
	 */
	static std::vector<Vector2> normals(const Vector2* points, int count)
	{
		float area = 0;
		for(int i = 0; i < count; i++)
		{
			const Vector2& p = points[i];
			const Vector2& q = points[(i + 1) % count];
			area += p.x * q.y - q.x * p.y;
		}
		float side = area > 0 ? 1 : -1;
		std::vector<Vector2> edges(count);
		for(int i = 0; i < count; i++)
		{
			const Vector2& p = points[i];
			const Vector2& q = points[(i + 1) % count];
			float dx = q.x - p.x, dy = q.y - p.y;
			float length = sqrtf(dx * dx + dy * dy);
			edges[i] = length > 0 ? Vector2{ side * dy / length, -side * dx / length } :
				Vector2{ 0, 0 };
		}
		std::vector<Vector2> result(count);
		for(int i = 0; i < count; i++)
		{
			Vector2 a = edges[(i + count - 1) % count], b = edges[i];
			Vector2 n = { a.x + b.x, a.y + b.y };
			float length = sqrtf(n.x * n.x + n.y * n.y);
			if(length == 0)
				continue;
			n.x /= length;
			n.y /= length;
			// miter: 1 / cos(meio ângulo), limitado nos cantos agudos
			float cosine = n.x * b.x + n.y * b.y;
			float miter = cosine > 0.25f ? 1 / cosine : 4;
			result[i] = Vector2{ n.x * miter, n.y * miter };
		}
		return result;
	}

	void fillPolygon(const Vector2* points, int count, Color color)
	{
		if(count < 3)
			return;
		std::vector<Vector2> n = normals(points, count);
		std::vector<Vector2> inner(count), outer(count);
		Vector2 center = { 0, 0 };
		for(int i = 0; i < count; i++)
		{
			inner[i] = add(points[i], n[i], -feather / 2);
			outer[i] = add(points[i], n[i], feather / 2);
			center.x += points[i].x / count;
			center.y += points[i].y / count;
		}
		Color edge = transparent(color);
		rlBegin(RL_TRIANGLES);
		for(int i = 0; i < count; i++)
		{
			int j = (i + 1) % count;
			triangle(center, color, inner[i], color, inner[j], color);
			quad(inner[i], color, outer[i], edge, outer[j], edge, inner[j], color);
		}
		rlEnd();
	}

	/**
	 * Faixa ao longo de um contorno, de offset - half a offset + half na
	 * direção das normais, suavizada dos dois lados
	 */
	static void band(const Vector2* points, const Vector2* n, int count, bool closed,
		float half, Color color)
	{
		// uma linha mais fina que o pixel fica mais transparente
		float solid = half - feather / 2;
		if(solid < 0)
		{
			color.a = (unsigned char)(color.a * (2 * half / feather));
			solid = 0;
		}
		float edge = solid + feather;
		Color clear = transparent(color);
		int segments = closed ? count : count - 1;
		rlBegin(RL_TRIANGLES);
		for(int i = 0; i < segments; i++)
		{
			int j = (i + 1) % count;
			Vector2 ai = add(points[i], n[i], solid), aj = add(points[j], n[j], solid);
			Vector2 bi = add(points[i], n[i], -solid), bj = add(points[j], n[j], -solid);
			Vector2 ci = add(points[i], n[i], edge), cj = add(points[j], n[j], edge);
			Vector2 di = add(points[i], n[i], -edge), dj = add(points[j], n[j], -edge);
			if(solid > 0)
				quad(ai, color, aj, color, bj, color, bi, color);
			quad(ai, color, ci, clear, cj, clear, aj, color);
			quad(bi, color, di, clear, dj, clear, bj, color);
		}
		rlEnd();
	}

	void line(Vector2 a, Vector2 b, float thickness, Color color)
	{
		float dx = b.x - a.x, dy = b.y - a.y;
		float length = sqrtf(dx * dx + dy * dy);
		if(length == 0)
			return;
		Vector2 points[2] = { a, b };
		Vector2 n[2] = { { -dy / length, dx / length }, { -dy / length, dx / length } };
		band(points, n, 2, false, thickness / 2, color);
	}

	void polyline(const Vector2* points, int count, float thickness, Color color)
	{
		for(int i = 0; i + 1 < count; i++)
			line(points[i], points[i + 1], thickness, color);
		// juntas e pontas arredondadas
		for(int i = 0; i < count; i++)
			fillCircle(points[i], thickness / 2, color);
	}

	/**
	 * Pontos de um círculo, mais pontos nos círculos maiores
	 */
	static std::vector<Vector2> circlePoints(Vector2 center, float radius)
	{
		int count = (int)(radius * 1.5f);
		if(count < 12)
			count = 12;
		if(count > 96)
			count = 96;
		std::vector<Vector2> points(count);
		for(int i = 0; i < count; i++)
		{
			float angle = 2 * PI * i / count;
			points[i] = Vector2{ center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
		}
		return points;
	}

	void fillCircle(Vector2 center, float radius, Color color)
	{
		std::vector<Vector2> points = circlePoints(center, radius);
		fillPolygon(points.data(), (int)points.size(), color);
	}

	void strokeCircle(Vector2 center, float radius, float thickness, Color color)
	{
		std::vector<Vector2> points = circlePoints(center, radius);
		std::vector<Vector2> n = normals(points.data(), (int)points.size());
		band(points.data(), n.data(), (int)points.size(), true, thickness / 2, color);
	}

	static std::vector<Vector2> roundedRectPoints(Rectangle rect, float radius)
	{
		float limit = (rect.width < rect.height ? rect.width : rect.height) / 2;
		if(radius > limit)
			radius = limit;
		if(radius < 0)
			radius = 0;
		int steps = radius > 0 ? (int)(radius / 1.5f) + 3 : 0;
		const Vector2 centers[4] = {
			{ rect.x + rect.width - radius, rect.y + radius },
			{ rect.x + rect.width - radius, rect.y + rect.height - radius },
			{ rect.x + radius, rect.y + rect.height - radius },
			{ rect.x + radius, rect.y + radius },
		};
		std::vector<Vector2> points;
		for(int corner = 0; corner < 4; corner++)
		{
			float start = -PI / 2 + corner * PI / 2;
			for(int i = 0; i <= steps; i++)
			{
				float angle = start + (PI / 2) * (steps > 0 ? (float)i / steps : 0);
				points.push_back(Vector2{ centers[corner].x + cosf(angle) * radius,
					centers[corner].y + sinf(angle) * radius });
				if(steps == 0)
					break;
			}
		}
		return points;
	}

	void fillRoundedRect(Rectangle rect, float radius, Color color)
	{
		std::vector<Vector2> points = roundedRectPoints(rect, radius);
		fillPolygon(points.data(), (int)points.size(), color);
	}

	void strokeRoundedRect(Rectangle rect, float radius, float thickness, Color color)
	{
		float inset = thickness / 2;
		Rectangle inner = { rect.x + inset, rect.y + inset, rect.width - thickness,
			rect.height - thickness };
		std::vector<Vector2> points = roundedRectPoints(inner, radius - inset);
		std::vector<Vector2> n = normals(points.data(), (int)points.size());
		band(points.data(), n.data(), (int)points.size(), true, inset, color);
	}
}
