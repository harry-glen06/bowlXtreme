enum GameState:
    AIMING
    ROLLING
    RESULT

class Game:
    private:
        state : GameState

        laneRect : Rectangle (position, size)
        score : int

        ball : Ball
        pins : list of Pin

        aimAngleDeg : float
        aimSpeedDeg : float
        launchSpeed : float

    public:
        Game(windowWidth, windowHeight)

        resetGame()
        resetPins()

        handleInput(dt)
        update(dt)
        checkCollisions()
        draw(window)
