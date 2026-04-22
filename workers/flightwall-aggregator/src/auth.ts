const encoder = new TextEncoder();

export async function verifyBearerAuth(
  request: Request,
  env: Env,
): Promise<boolean> {
  const header = request.headers.get("Authorization");
  if (header === null) {
    return false;
  }

  const match = /^Bearer\s+(.+)$/i.exec(header);
  if (match === null) {
    return false;
  }

  const provided = match[1].trim();
  if (provided.length === 0) {
    return false;
  }

  const [providedDigest, expectedDigest] = await Promise.all([
    crypto.subtle.digest("SHA-256", encoder.encode(provided)),
    crypto.subtle.digest("SHA-256", encoder.encode(env.ESP32_AUTH_TOKEN)),
  ]);

  return constantTimeEqual(
    new Uint8Array(providedDigest),
    new Uint8Array(expectedDigest),
  );
}

function constantTimeEqual(left: Uint8Array, right: Uint8Array): boolean {
  if (left.byteLength !== right.byteLength) {
    return false;
  }

  let diff = 0;
  for (let index = 0; index < left.byteLength; index += 1) {
    diff |= left[index] ^ right[index];
  }

  return diff === 0;
}
