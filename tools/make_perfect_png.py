from PIL import Image

def remove_background_perfect(in_path, out_path):
    im = Image.open(in_path).convert("RGBA")
    w, h = im.size
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))

    TH_HIGH = 225
    TH_LOW = 160

    for y in range(h):
        for x in range(w):
            r, g, b, _ = im.getpixel((x, y))
            min_c = min(r, g, b)

            if min_c >= TH_HIGH:
                # 100% transparent background
                out.putpixel((x, y), (0, 0, 0, 0))
            elif min_c <= TH_LOW:
                # 100% opaque house pixel
                out.putpixel((x, y), (r, g, b, 255))
            else:
                # Smooth anti-aliased edge
                alpha = int(255 * (TH_HIGH - min_c) / (TH_HIGH - TH_LOW))
                alpha = max(0, min(255, alpha))
                if alpha <= 4:
                    out.putpixel((x, y), (0, 0, 0, 0))
                else:
                    # Unmultiply white to prevent fringe
                    true_r = max(0, min(255, int((r * 255 - (255 - alpha) * 255) / alpha)))
                    true_g = max(0, min(255, int((g * 255 - (255 - alpha) * 255) / alpha)))
                    true_b = max(0, min(255, int((b * 255 - (255 - alpha) * 255) / alpha)))
                    out.putpixel((x, y), (true_r, true_g, true_b, alpha))

    # Also trim excess empty margins at top/bottom/left/right so the vector looks sharp and big in the header!
    bbox = out.getbbox()
    print("Clean BBox:", bbox)
    
    # Crop to bbox with 4px margin
    pad = 4
    crop_box = (
        max(0, bbox[0] - pad),
        max(0, bbox[1] - pad),
        min(w, bbox[2] + pad),
        min(h, bbox[3] + pad)
    )
    cropped = out.crop(crop_box)
    print("Cropped size:", cropped.size)
    cropped.save(out_path, format="PNG")
    print(f"Saved {out_path}")
    return cropped

clean_im = remove_background_perfect(
    "C:/Users/Nirvana/.gemini/antigravity-cli/brain/9d5764a7-6847-4208-99f4-3d9c9f69e5dd/.user_uploaded/uploaded_media_1795453284078.png",
    "web_src/joglo.png"
)

# Test composite on #F8F6F1 canvas
canvas = Image.new("RGBA", clean_im.size, (248, 246, 241, 255))
comp = Image.alpha_composite(canvas, clean_im)
comp.save("tools/perfect_preview.png")
print("Saved tools/perfect_preview.png")

# Check former trapped space in original coords (45, 222):
# Note: cropped offset is crop_box[0], crop_box[1]
