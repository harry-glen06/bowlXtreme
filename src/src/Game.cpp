Game constructor:
    set up laneRect (centered, long rectangle)
    set default values:
        score = 0
        state = AIMING
        aimAngleDeg = -90
        aimSpeedDeg = (some number like 140)
        launchSpeed = (some number like 650)

    set ball start position near bottom of lane
    ball.reset(ballStart)

    resetPins()

resetGame():
    score = 0
    state = AIMING
    aimAngleDeg = -90
    ball.reset(ballStart)
    resetPins()

resetPins():
    clear pins list

    find laneCenterX
    choose startY near top of lane
    spacing = 28

    build triangle rows:
        row 1: 1 pin
        row 2: 2 pins
        row 3: 3 pins
        row 4: 4 pins

    for each row:
        place pins centered around laneCenterX
        add Pin(position) to pins list

handleInput(dt):
    if key R pressed:
        resetGame()

    if state == AIMING:
        if Left pressed:
            aimAngleDeg -= aimSpeedDeg * dt
        if Right pressed:
            aimAngleDeg += aimSpeedDeg * dt

        clamp aimAngleDeg to stay mostly pointing up lane
            for example: between -140 and -40

        if Space pressed:
            direction = vector(cos(angle), sin(angle)) using aimAngleDeg
            ball.launch(direction, launchSpeed)
            state = ROLLING

update(dt):
    if state == ROLLING:
        ball.update(dt)

        keep ball inside lane:
            if ball outside left or right:
                push it back inside
                optionally flip x velocity a little
            same for top/bottom

        checkCollisions()

        if ball speed is very small:
            ball.stop()
            state = RESULT

checkCollisions():
    for each pin:
        if pin is already knocked: continue

        if distance(ball.pos, pin.pos) < ball.radius + pin.radius:
            pin.knock()
            score += 1

draw(window):
    draw background
    draw laneRect

    for each pin:
        if not knocked:
            pin.draw(window)

    ball.draw(window)

    if state == AIMING:
        draw an aim line from ball position in the aim direction

    draw score text if you want
