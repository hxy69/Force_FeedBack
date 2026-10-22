# utils/mapping.py

def height_to_canvas_y(height, max_height, canvas_height):
    """
    Map physical height -> canvas Y coordinate
    """
    if canvas_height <= 0:
            return 0
    return canvas_height - (height / max_height) * canvas_height
