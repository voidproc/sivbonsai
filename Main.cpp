#include <Siv3D.hpp>


/*  SIVBONSAI（シブ盆栽）  @voidproc  */


// フィールドのサイズ
const int W = 320;
const int H = 240;

// 拡大率
const double SCALE = 2.0;

// キー入力に応じてプレイヤーを移動させる
// 戻り値 :
//   -1 (left)
//   +1 (right)
int movePlayer(Rect& player)
{
	double dx = 1.0;

	if (Input::KeyLeft.pressed)
	{
		player.x -= dx;
		return -1;
	}
	if (Input::KeyRight.pressed)
	{
		player.x += dx;
		return 1;
	}

	return 0;
}

// キー入力に応じて武器を回転させる
// 戻り値 :
//   true (rotated)
bool rotateWeaponDir(double& dir)
{
	if (Input::KeyA.pressed)
	{
		dir = Clamp(dir - 0.050, -Math::HalfPi, Math::HalfPi);
		return true;
	}
	if (Input::KeyD.pressed)
	{
		dir = Clamp(dir + 0.050, -Math::HalfPi, Math::HalfPi);
		return true;
	}

	return false;
}

// Effect

struct StarFx : IEffect
{
	StarFx(const Point& pos) : pos_(pos), rot_(Random(Math::TwoPi))
	{
	}

	bool update(double t) override
	{
		rot_ += 0.05;

		Geometry2D::CreateStar((1.5 - t) * 6.0, rot_).movedBy(pos_).draw(Color(Palette::White, 240 * Saturate(1.0 - t)));

		return t < 1.0;
	}

	Point pos_;
	double rot_;
};

struct ExplodeFx : IEffect
{
	ExplodeFx(const Point& pos) : pos_(pos)
	{
	}

	bool update(double t) override
	{
		Circle(Random(8.0, 20.0) * (1.0 - t)).movedBy(pos_).drawFrame(2.0, 0.0, Color(Palette::White, (1.00 - t) * (100 + 128 * ((System::FrameCount() / 2) % 2))));
		return t < 0.75;
	}

	Point pos_;
};

// Enemy

enum class EnemyType
{
	Stone,
	Ball,
	Bigstone,
};

struct Enemy
{
	Enemy(const EnemyType type, const Vec2& pos, const Vec2& vel, const int life = 1)
		: type_(type), pos_(pos), vel_(vel), life_(life)
	{
	}

	void update()
	{
		pos_ += vel_;

		if (type_ == EnemyType::Stone)
		{
			TextureAsset(L"stone")(16 * ((System::FrameCount() / 16) % 2), 0, 16, 16).drawAt(pos_);
		}
		else if (type_ == EnemyType::Ball)
		{
			TextureAsset(L"ball")(16 * ((System::FrameCount() / 16) % 2), 0, 16, 16).drawAt(pos_);
		}
		else if (type_ == EnemyType::Bigstone)
		{
			const uint32 gb = 128 + 127 * ((System::FrameCount() / 4) % 2);
			TextureAsset(L"stone")(16 * ((System::FrameCount() / 16) % 2), 0, 16, 16).scale(2).drawAt(pos_, Color(255,gb,gb));
		}
	}

	RectF collision()
	{
		if (type_ == EnemyType::Bigstone)
		{
			return{ pos_.movedBy(-10, -10), 20, 20 };
		}
		else
		{
			return{ pos_.movedBy(-5, -5), 10, 10 };
		}
	}

	EnemyType type_;
	Vec2 pos_;
	Vec2 vel_;
	int life_;
};

enum class GameState
{
	Title,
	Main,
	Over,
};

void Main()
{
	Window::SetTitle(L"SIVBONSAI - Siv3D Game Jam 第13回 テーマ「草」投稿作品");

	Window::Resize(W * SCALE, H * SCALE);

	Graphics::SetBackground(Palette::Black);

	Graphics2D::SetSamplerState(SamplerState::WrapPoint);

	RenderTexture renderTexture(W, H, Palette::Deepskyblue);

	Effect effect;

	// Font

	FontManager::Register(L"Assets/8bitoperator_jve.ttf");
	const Font font(8, L"8bitoperator JVE");
	const Font fontM(16, L"8bitoperator JVE");
	const Font fontL(24, L"8bitoperator JVE");

	// Texture

	Texture txSand(L"Assets/sand.png");
	Texture txPot(L"Assets/pot.png");
	Texture txBonsai(L"Assets/bonsai.png");
	Texture txPlayer(L"Assets/player.png");
	Texture txBG(L"Assets/bg.png");
	TextureAsset::Register(L"stone", L"Assets/stone.png");
	TextureAsset::Register(L"ball", L"Assets/ball.png");

	// Player

	Rect player(20, 0, 24, 32);
	double weaponDir = 0;
	double weaponWidth = 40;
	bool playerWalkRight = true;
	Stopwatch stopwatchWeapon(true);

	// Bonsai

	Rect pot(W / 2 - 32 / 2, 0, 32, 24);
	Rect bonsai(0, 0, 32, 32);

	// Enemy

	Array<Enemy> enemy;

	// Game state

	GameState state = GameState::Title;
	int price = 0;
	int frame = 0;

	while (System::Update())
	{ 
		Graphics2D::SetRenderTarget(renderTexture);

		if (state == GameState::Title)
		{
			// BG

			Rect(W, H).draw(Palette::Green);

			// Title text

			fontL(L"S I V B O N S A I").drawCenter(Rect(W, H).center.movedBy(0+2, -20+2), Color(Palette::Darkred, 100 + 128 * ((frame / 2) % 2)));
			fontL(L"S I V B O N S A I").drawCenter(Rect(W, H).center.movedBy(0, -20), Palette::White);

			if ((frame % 30) > 10)
			{
				font(L"Press ENTER / SPACE key").drawCenter(Rect(W, H).center.movedBy(0, +40), Palette::White);
			}

			if (Input::KeySpace.clicked || Input::KeyEnter.clicked)
			{
				state = GameState::Main;
				frame = 0;
			}
		}
		else if (state == GameState::Over)
		{
			// BG

			Rect(W, H).draw(Color(Palette::Darkred, 5));

			// Result
			
			fontL(L"G A M E  O V E R").drawCenter(Rect(W, H).center.movedBy(0, -40), Palette::White);
			fontM(L"PRICE: $", price, L"000").drawCenter(Rect(W, H).center.movedBy(0, +10), Palette::Gold);

			if (frame > 60 * 3)
			{
				if ((frame % 30) > 10)
				{
					font(L"Press ENTER / SPACE key").drawCenter(Rect(W, H).center.movedBy(0, +50), Palette::White);
				}

				if (Input::KeySpace.clicked || Input::KeyEnter.clicked)
				{
					// Reset game
					frame = 0;
					price = 0;
					player.x = 20;
					playerWalkRight = true;
					weaponDir = 0;
					weaponWidth = 40;
					stopwatchWeapon.restart();
					enemy.clear();
					state = GameState::Main;
				}
			}
		}
		else
		{
			// BG

			txBG.map(W, H).draw(Color(220));

			// Ground

			const int groundY = 180;

			txSand.map(W, H - groundY).draw(0, groundY);

			// Bonsai pot

			pot.y = groundY - pot.h;

			// Bonsai

			const double bonsaiScale = 1.0 + 0.1 * (frame / 600);  // Grow

			Rect bonsaiRect(pot.x + pot.w / 2 - bonsai.w * bonsaiScale / 2,
				pot.y - bonsai.h * bonsaiScale + 1,
				bonsai.w * bonsaiScale,
				bonsai.h * bonsaiScale);
			bonsaiRect(txBonsai).draw();

			Circle bonsaiCollision(bonsaiRect.center, bonsai.w * bonsaiScale / 2.0);

			pot(txPot).draw();

			// Player

			if (const int dir = movePlayer(player))
			{
				playerWalkRight = (dir == 1);
			}

			player.y = groundY - player.h;

			const TextureRegion tx = txPlayer(24 * ((frame / 20) % 2), 0, 24, 32);

			if (playerWalkRight)
			{
				tx.draw(player.pos.movedBy(1, 0), Color(Palette::Black, 128));
				tx.draw(player.pos);
			}
			else
			{
				tx.mirror().draw(player.pos.movedBy(1, 0), Color(Palette::Black, 128));
				tx.mirror().draw(player.pos);
			}

			Rect playerCollision(player.x + 7, player.y + 2, 14, 30);

			if (!playerWalkRight)
			{
				playerCollision.moveBy(-4, 0);
			}

			// Player weapon direction

			if (rotateWeaponDir(weaponDir) || ((frame / 20) % 2))
			{
				Line(player.center + Circular(30, weaponDir), player.center + Circular(100, weaponDir))
					.drawArrow(4.0, { 15.0, 15.0 }, Palette::Red);
			}

			// Player weapon

			Optional<Quad> weapon = none;

			if (stopwatchWeapon.ms() > 200)
			{
				if (Input::KeySpace.released)
				{
					stopwatchWeapon.restart();
				}

				if (Input::KeySpace.pressed)
				{
					weapon = Rect(weaponWidth, 1000)
						.rotated(weaponDir)
						.movedBy(player.center.x - weaponWidth / 2, player.center.y - 500);

					weapon.value()
						.drawFrame(5.0, Color(Palette::White, 64 + 64 * ((frame / 2) % 2)))
						.draw(Color(Palette::White, 128 + 127 * ((frame / 3) % 2)));

					if ((frame / 3) % 2)
						effect.add<StarFx>(player.center + Point(Random(-weaponWidth, weaponWidth), Random(-weaponWidth, weaponWidth)) + Vec2(Circular(Random(-100, 400), weaponDir)).asPoint());

					weaponWidth = Clamp(weaponWidth - 0.55, 5.0, 50.0);
				}
				else
				{
					weaponWidth = Clamp(weaponWidth + 0.40, 5.0, 50.0);
				}
			}

			// Enemy

			if ((frame % Max(30, (60 - 3 * (frame / 600)))) == 0)
			{
				Vec2 enemyPos = { RandomSelect({ -16, W + 16 }), Random(60, 140) };
				Vec2 enemyVel = Circular(Random(1.0, 1.5) + 0.02 * (frame / 600), Random(60_deg, 120_deg));
				if (enemyPos.x > 0) enemyVel.x = -enemyVel.x;

				enemy.push_back(Enemy(RandomSelect({ EnemyType::Stone, EnemyType::Ball }), enemyPos, enemyVel));
			}

			// Boss
			if ((frame % (30 * 60)) == 30 * 60 - 1)
			{
				Vec2 enemyPos = { RandomSelect({ -16, W + 16 }), Random(30, 100) };
				Vec2 enemyVel = Circular(0.7 + 0.02 * (frame / 600), Random(110_deg, 140_deg));
				if (enemyPos.x > 0) enemyVel.x = -enemyVel.x;

				enemy.push_back(Enemy(EnemyType::Bigstone, enemyPos, enemyVel, 30));
			}

			for (auto& e : enemy)
			{
				e.update();
			}

			// Check collision & remove enemy

			if (weapon)
			{
				for (auto& e : enemy)
				{
					if (weapon.value().intersects(e.collision()))
					{
						--e.life_;
					}
				}

				Erase_if(enemy, [&](auto& e)
				{
					if (e.life_ <= 0)
					{
						effect.add<ExplodeFx>(e.pos_.asPoint());
						return true;
					}
					return false;
				});
			}

			Erase_if(enemy, [](auto& e) { return !Rect(-32, -32, W + 32 * 2, H + 32 * 2).intersects(e.collision()); });

			// Over ?

			for (auto& e : enemy)
			{
				if (playerCollision.intersects(e.collision()) || bonsaiCollision.intersects(e.collision()))
				{
					state = GameState::Over;
					frame = 0;
				}
			}

			// Effect

			effect.update();

			// Status

			price += 1;
			if (!weapon)
				price += (uint32)Pow(frame / 150, 1.2);

			font(L"PRICE: $", price, L"000").draw({ 1, 0 });
			font(L"RANK: ", 1 + frame / 600).draw({ 1, 20 });
		}

		Graphics2D::SetRenderTarget(Graphics::GetSwapChainTexture());
		renderTexture.scale(SCALE).draw();

		++frame;
	}
}
