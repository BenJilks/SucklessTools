use crate::buffer::rune::StandardColor;
use crate::buffer::DrawAction;
use crate::display::{Event, ResizeEvent};
use raylib::prelude::*;
use std::sync::mpsc;

const FONT_SIZE: i32 = 30;

struct ScreenBuffer {
    front: RenderTexture2D,
    back: RenderTexture2D,
}

fn handle_input(rl: &mut RaylibHandle, event_sender: &mpsc::Sender<Event>) {
    if let Some(char) = rl.get_char_pressed() {
        let mut buffer = vec![0u8; 10];
        let _ = event_sender.send(Event::input_str(char.encode_utf8(&mut buffer)));
    }

    if rl.is_key_pressed(KeyboardKey::KEY_ENTER) {
        let _ = event_sender.send(Event::input_str("\n"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_BACKSPACE) {
        let _ = event_sender.send(Event::input_str("\x08"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_ESCAPE) {
        let _ = event_sender.send(Event::input_str("\x1b"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_TAB) {
        let _ = event_sender.send(Event::input_str("\t"));
    }

    if rl.is_key_pressed(KeyboardKey::KEY_UP) {
        let _ = event_sender.send(Event::input_str("\x1b[A"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_DOWN) {
        let _ = event_sender.send(Event::input_str("\x1b[B"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_RIGHT) {
        let _ = event_sender.send(Event::input_str("\x1b[C"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_LEFT) {
        let _ = event_sender.send(Event::input_str("\x1b[D"));
    }

    if rl.is_key_pressed(KeyboardKey::KEY_HOME) {
        let _ = event_sender.send(Event::input_str("\x1b[H"));
    }
    if rl.is_key_pressed(KeyboardKey::KEY_END) {
        let _ = event_sender.send(Event::input_str("\x1b[F"));
    }

    if rl.is_key_down(KeyboardKey::KEY_LEFT_CONTROL) {
        if rl.is_key_pressed(KeyboardKey::KEY_B) {
            let _ = event_sender.send(Event::Input(vec![2]));
        }
        if rl.is_key_pressed(KeyboardKey::KEY_C) {
            let _ = event_sender.send(Event::Input(vec![3]));
        }
        if rl.is_key_pressed(KeyboardKey::KEY_L) {
            let _ = event_sender.send(Event::Input(vec![12]));
        }
        if rl.is_key_pressed(KeyboardKey::KEY_R) {
            let _ = event_sender.send(Event::Input(vec![18]));
        }
    }
}

fn perform_draw_action(
    action: &DrawAction,
    font: &WeakFont,
    font_size: Vector2,
    display: &mut ScreenBuffer,
    draw: &mut RaylibDrawHandle,
    thread: &RaylibThread,
) {
    match action {
        DrawAction::ClearScreen => {
            let mut screen_draw = draw.begin_texture_mode(&thread, &mut display.back);
            screen_draw.clear_background(Color::BLACK);
        },

        DrawAction::Flush => {
            std::mem::swap(&mut display.back, &mut display.front);
        },

        DrawAction::Clear {
            attribute,
            row,
            column,
            width,
            height,
        } => {
            let mut screen_draw = draw.begin_texture_mode(&thread, &mut display.back);
            screen_draw.draw_rectangle_v(
                Vector2 {
                    x: *column as f32 * font_size.x,
                    y: *row as f32 * font_size.y,
                },
                Vector2 {
                    x: *width as f32 * font_size.x,
                    y: *height as f32 * font_size.y,
                },
                Color::get_color(attribute.background),
            );
        }

        DrawAction::Runes(runes) => {
            let mut screen_draw = draw.begin_texture_mode(&thread, &mut display.back);
            for (rune, cursor_pos) in runes {
                let position = Vector2 {
                    x: cursor_pos.get_column() as f32 * font_size.x,
                    y: cursor_pos.get_row() as f32 * font_size.y,
                };

                screen_draw.draw_rectangle_v(
                    position,
                    font_size,
                    Color::get_color(rune.attribute.background),
                );

                if rune.code_point != 0 {
                    screen_draw.draw_text_codepoint(
                        font,
                        rune.code_point as i32,
                        position,
                        FONT_SIZE as f32,
                        Color::get_color(rune.attribute.foreground),
                    );
                }
            }
        }

        _ => {}
    }
}

fn create_resize_event(rl: &RaylibHandle, font_size: Vector2) -> ResizeEvent {
    ResizeEvent {
        rows: rl.get_screen_height() / font_size.y as i32,
        columns: rl.get_screen_width() / font_size.x as i32,
        width: rl.get_screen_width(),
        height: rl.get_screen_height(),
    }
}

fn load_font(rl: &mut RaylibHandle, thread: &RaylibThread) -> (WeakFont, Vector2) {
    let font = rl
        .load_font_ex(
            &thread,
            "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
            FONT_SIZE,
            None,
        )
        .map(|x| x.make_weak())
        .unwrap_or(rl.get_font_default());

    let font_size = font.measure_text(" ", FONT_SIZE as f32, 0.0);
    (font, font_size)
}

fn handle_resize(
    rl: &mut RaylibHandle,
    thread: &RaylibThread,
    display: &mut ScreenBuffer,
    font_size: Vector2,
    event_sender: &mpsc::Sender<Event>,
) {
    let event = create_resize_event(&rl, font_size);

    let screen_width = event.columns * font_size.x as i32;
    let screen_height = event.rows * font_size.y as i32;
    if display.front.width() != screen_width || display.front.height() != screen_height {
        display.front = rl
            .load_render_texture(&thread, screen_width as u32, screen_height as u32)
            .expect("Can create screen render target");
        display.back = rl
            .load_render_texture(&thread, screen_width as u32, screen_height as u32)
            .expect("Can create screen render target");
    }

    let _ = event_sender.send(Event::Resize(event));
}

pub fn start_raylib_display(
    title: &str,
    event_sender: mpsc::Sender<Event>,
    draw_action_receiver: mpsc::Receiver<DrawAction>,
) {
    let (mut rl, thread) = raylib::init()
        .title(title)
        .size(1600, 800)
        .resizable()
        .build();
    rl.set_target_fps(60);
    rl.set_exit_key(None);

    let (font, font_size) = load_font(&mut rl, &thread);
    let mut display = ScreenBuffer {
        front: rl
            .load_render_texture(
                &thread,
                rl.get_screen_width() as u32,
                rl.get_screen_height() as u32,
            )
            .expect("Can create screen render target"),
        back: rl
            .load_render_texture(
                &thread,
                rl.get_screen_width() as u32,
                rl.get_screen_height() as u32,
            )
            .expect("Can create screen render target"),
    };

    // NOTE: Set initial size.
    handle_resize(&mut rl, &thread, &mut display, font_size, &event_sender);

    while !rl.window_should_close() {
        handle_input(&mut rl, &event_sender);
        if rl.is_window_resized() {
            handle_resize(&mut rl, &thread, &mut display, font_size, &event_sender);
        }

        let mut draw = rl.begin_drawing(&thread);
        draw.clear_background(Color::get_color(StandardColor::DefaultBackground.color()));

        while let Ok(action) = draw_action_receiver.try_recv() {
            if let DrawAction::Close = action {
                return;
            }

            perform_draw_action(&action, &font, font_size, &mut display, &mut draw, &thread);
        }

        draw.draw_texture_rec(
            &display.front,
            Rectangle {
                x: 0.0,
                y: 0.0,
                width: display.front.width() as f32,
                height: -display.front.height() as f32,
            },
            Vector2 { x: 0.0, y: 0.0 },
            Color::WHITE,
        );
    }
}
